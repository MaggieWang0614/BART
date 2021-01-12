#define BOOST_MATH_OVERFLOW_ERROR_POLICY errno_on_error

#include "mlbartfuns.h"
#include <boost/math/special_functions/bessel.hpp>



using namespace boost::math;

//compute r = \sum y_ij and s = \sum phi_i f_(t)(x) for left and right give bot and v,c
void mlgetsuff(tree& x, tree::tree_p nx, size_t v, size_t c, xinfo& xi, mlogitdinfo& mdi, size_t& nl, double& syl, size_t& nr, double& syr)
{
   double *xx;//current x
   nl=0; syl=0.0;
   nr=0; syr=0.0;

   for(size_t i=0;i<mdi.n;i++) {
      if (mdi.y[i] != mdi.ik) {continue;}
      xx = mdi.x + i*mdi.p;

      if (isnan(mdi.phi[i])){cout << "phi_" << i << " is nan" << endl; exit(1);}
      if (isnan(mdi.f[mdi.ik*mdi.n + i])){cout << "mdi.f_" << i << " is nan" << ", class " << mdi.ik << endl; exit(1);}

      if(nx==x.bn(xx,xi)) { //does the bottom node = xx's bottom node
         if(xx[v] < xi[v][c]) {
            nl++; // does xx belong to category ik
            syl += mdi.phi[i] * mdi.f[mdi.ik*mdi.n + i]; // mdi.f = allfit = f_(h)
          } else {
            nr++;
            syr += mdi.phi[i] * mdi.f[mdi.ik*mdi.n + i];
          }
      }
   }

}

//--------------------------------------------------
//compute n and \sum y_i for left and right bots
void mlgetsuff(tree& x, tree::tree_p l, tree::tree_p r, xinfo& xi, mlogitdinfo& mdi, size_t& nl, double& syl, size_t& nr, double& syr)
{
   double *xx;//current x
   nl=0; syl=0.0;
   nr=0; syr=0.0;

   for(size_t i=0;i<mdi.n;i++) {
      if (mdi.y[i] != mdi.ik) {continue;} // only count currecnt category
      xx = mdi.x + i*mdi.p;
      tree::tree_cp bn = x.bn(xx,xi);

      if (isnan(mdi.phi[i])){cout << "phi_" << i << " is nan" << endl; exit(1);}
      if (isnan(mdi.f[mdi.ik*mdi.n + i])){cout << "mdi.f_" << i << " is nan" << ", class " << mdi.ik << endl; exit(1);}

      if(bn==l) {
        nl++;
        syl += mdi.phi[i] * mdi.f[mdi.ik*mdi.n + i];
      }
      if(bn==r) {
        nr++;
        syr += mdi.phi[i] * mdi.f[mdi.ik*mdi.n + i];
      }
   }
}

//--------------------------------------------------
//get sufficients stats for all bottom nodes, this way just loop through all the data once.
void mlallsuff(tree& x, xinfo& xi, mlogitdinfo& mdi, tree::npv& bnv, std::vector<size_t>& nv, std::vector<double>& syv)
{
   tree::tree_cp tbn; //the pointer to the bottom node for the current observations
   size_t ni;         //the  index into vector of the current bottom node
   double *xx;        //current x

   bnv.clear();
   x.getbots(bnv);

   typedef tree::npv::size_type bvsz;
   bvsz nb = bnv.size();
   nv.resize(nb);
   syv.resize(nb);

   std::map<tree::tree_cp,size_t> bnmap;
   for(bvsz i=0;i!=bnv.size();i++) {bnmap[bnv[i]]=i;nv[i]=0;syv[i]=0.0;}

   for(size_t i=0;i<mdi.n;i++) {
      if (mdi.y[i] != mdi.ik) {continue;}
      xx = mdi.x + i*mdi.p;
      tbn = x.bn(xx,xi);
      ni = bnmap[tbn];

      if (isnan(mdi.phi[i])){cout << "phi_" << i << " is nan" << endl; exit(1);}
      if (isnan(mdi.f[mdi.ik*mdi.n + i])){cout << "mdi.f_" << i << " is nan" << ", class " << mdi.ik << endl; exit(1);}

      ++(nv[ni]);
      syv[ni] += mdi.phi[i] * mdi.f[mdi.ik*mdi.n + i];
   }
}
//--------------------------------------------------
// draw all the bottom node lambda's
void drlamb(tree& t, xinfo& xi, mlogitdinfo& mdi, mlogitpinfo& mpi, rn& gen)
{
   tree::npv bnv;
   std::vector<size_t> nv;
   std::vector<double> syv;
   mlallsuff(t,xi,mdi,bnv,nv,syv);

   for(tree::npv::size_type i=0;i!=bnv.size();i++) 
      bnv[i]->settheta(drawnodelambda(nv[i],syv[i],mpi.c,mpi.d,gen));
}

//lh, replacement for lil that only depends on sum y.
double mllh(size_t n, double sy, double c, double d, double z3)
{
    double z1 = gignorm(-c + n, 2*d, 2*sy);
    double z2 = gignorm(c + n, 0, 2*(d + sy));
    // double z3 = gignorm(c, 0, 2*d); // should be predefined
   return log((z1 + z2) / 2 / z3);
}
//--------------------------------------------------
//draw one lambda from post 
double drawnodelambda(size_t n, double sy, double c, double d, rn& gen)
{
    // lambda ~ pi*GIG(-c+r, 2d, 2s) + (1-pi)*Gamma(c+r, d+s)
    // pi = Z(-c+r, 2*d, 2*s) / (Z(-c+r, 2d, 2s) + Z(c+r, 0, 2*(d+s)))
    // r = n, s = sy
    cout << "get pi"<<endl;
    double z1 = gignorm(-c+n, 2*d, 2*sy);
    double z2 = gignorm(c+n, 0, 2*(d+sy));
    double _pi =  z1 / (z1+z2);
    if (gen.uniform() < _pi){ // 
    cout <<"draw from gig(-c+r, 2*d, 2*s)" <<endl;
        double eta = -c + n; 
        double chi = 2*d;
        double psi = 2*sy;
    cout << "eta = " << eta << ", chi = " << chi << ", psi = " << psi << endl;
        // draw u1, u2 independetly from U(0, ib), U(0, id)
        // ib = sup sqrt(h(x))
        double bx = psi == 0 ? chi / (2-2*eta) : (sqrt(pow(eta, 2) - 2*eta + chi * psi + 1) + eta - 1) / psi;
        double ib = sqrt(exp(lgigkernal(bx, eta, chi, psi)));
        // id = sup x*sqrt(h(x))
        double dx = psi == 0 ? -chi / 2 / (eta + 1) : (sqrt(pow(eta, 2) + 2*eta + chi*psi + 1) + eta + 1) / psi;
        double id = dx * sqrt(exp(lgigkernal(dx, eta, chi, psi)));
        size_t num_try = 0;
        while (true)
        {
            double u1 = gen.uniform()*ib;
            double u2 = gen.uniform()*id;
            if (isinf(u1) | isinf(u2)) {cout << "u1 = " << u1 << ", u2 = " << u2 << endl; exit(1);}
            if (2*log(u1) <= lgigkernal(u2/u1, eta, chi, psi)) { return u2 / u1; }
            else {num_try += 1;}
            if (num_try == 1000){
                cout << "Warning: Sampling lambda exceeds 1000 iterations." << endl;
                cout << "ib = " << ib << ", id = " << id << endl;
                return u2/u1; 
            }
        }
    }
    else { // draw from gig(c+r, 0, 2*(d+s)) or equivalently gamma(c+r, d+s)
        return gen.gamma(c+n, d+sy);
    }
}
void drphi(double *phi, double *allfit, size_t n, size_t k, rn& gen)
{
    double sum_fit; 
    for (size_t i = 0; i < n; i++){
        sum_fit = 0.0;
        for (size_t j = 0; j < k; j++){
            sum_fit += allfit[j*n + i];
        }
        phi[i] = gen.gamma(1, sum_fit); 
        if (isnan(phi[i])) {cout << "phi_" << i << " is nan, sum_fit = " << sum_fit << endl; exit(1);}
    }
}
// return the nomalization term for generazlied inverse gaussian (gig) distribution, see mlnomial BART, Jared Murray
double gignorm(double eta, double chi, double psi) 
{ 
    if (isinf(psi)){
        cout << "psi = inf" << endl; exit(1);
    }
    double ret;
    if ((eta > 0)&&(chi==0)&&(psi>0)){
        ret = exp(lgamma(eta) + eta * log(2 / psi));
    }else if ((eta < 0)&&(chi>0)&&(psi==0)){
        ret = exp(lgamma(-eta) - eta * log(2 / chi));
    }else if ((chi>0)&&(psi>0)){
        double bessel_k = cyl_bessel_k(eta, sqrt(chi*psi));
        ret = exp(log(2*bessel_k) - (eta / 2) * log(psi / chi));
    }else{
        cout << "Warning: no solution for gignorm with eta = "<< eta <<", chi = " << chi <<", psi = "<< psi << endl;
        exit(1);
    }
    return ret;
}
double lgigkernal(double x, double eta, double chi, double psi)
{
    // return pow(x, eta-1)*exp(-(chi/x + psi*x)/2);
    return (eta-1)*log(x) - (chi/x + psi*x)/2;
}
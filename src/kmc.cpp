#include "common.h"
#include <vector>
#include <Rcpp.h>
#include <iostream>


List RevCHECK(SEXP xx) {
  NumericMatrix x(xx);
  List re;
  NumericVector rr(1);
  if (x.nrow() !=2 ) {
    rr(0) = signcheck(x);
  }else{
      int sign_flg = 0;
      int sign_flg_ = 0;
      std::vector<int> pos_in(x.ncol());
      std::vector<int> neg_in(x.ncol());
      for (int j=0;j<x.ncol();j++) {
          if (x(0,j)>0) {pos_in[sign_flg]=j;sign_flg++;}
          if (x(0,j)<0) {neg_in[sign_flg_]=j;sign_flg_++;}
      }
      NumericMatrix Amat(1, sign_flg * sign_flg_ );
      int ind=0;
      for(int i = 0; i< sign_flg;i++){
          for(int j = 0; j<sign_flg_;j++){
              Amat(0,ind) = x(0,pos_in[i])*x(1,neg_in[j]) - x(0,neg_in[j])*x(1,pos_in[i]);
              ind++;
          }
      }
      rr(0) = signcheck(Amat);
  }
  re("flg") = rr;
  return(re);
}


List omegalambda(SEXP kmctime,SEXP delta,SEXP lambda,SEXP gtmat){
    /* 
    // kmctime: T
    // delta:   status
    // lambda:  root finding
    // gtmat:   g_1(X),...,g_p(X)
     */
Environment stats("package:stats");
RNGScope scope; // Don;t need rnd
NumericMatrix Gtmat(gtmat); // In R, gtmat is stored as a double[]
NumericVector Kmctime(kmctime),Delta(delta);
vector<double> Lambda=Rcpp::as< std::vector<double> >(lambda);

//int p=Gtmat.nrow();
int n=Gtmat.ncol();
double tmp=0.;
NumericVector S(n);
//NumericVector uncenloc(floor(sum(Delta)));
int cenlocL=n-floor(sum(Delta));
NumericVector cenloc(cenlocL);
int intflg=0;
for (int i=0;i<n;i++){
	if (Delta(i)<.5) {
        cenloc(intflg)=i;//==0
        intflg++;
    }
}
Delta(n-1)=1;//setting the last to be observed!

//start iteration
NumericVector uomega(n);
uomega(0)=1./((double)n-sum(Lambda,Gtmat,0));
double Scen=0.;
for (int k=1;k<n;k++){
	if (Delta(k)>.5){//==1
		tmp=0.;
		for (int i=0;i<n;i++){
			tmp+=uomega(i);
			S(i)=1-tmp;
		}
		Scen=0;
		if (cenloc(0)<(k-1)) {
			intflg=0;
			while (intflg<cenlocL  && cenloc(intflg)<=(k-1) ){
				//if (k>47990) printf("Check point: %d iteration %dINTFLG\n",k,intflg);
                Scen+=1/S(floor(cenloc(intflg)));
				intflg++;
			}
		}
	 uomega(k)=1./(((double)n)-sum(Lambda,Gtmat,k) -Scen);
	}
}
List re; re("S")=S;re("omega")=uomega;re("gt")=Gtmat;
return(re);
}

List RCPP_KMCDATA(SEXP kmctime,SEXP delta,SEXP lambda,SEXP gtmat){
    Environment stats("package:stats");
    RNGScope scope;
    NumericMatrix Gtmat(gtmat);
    NumericVector Kmctime(kmctime),Delta(delta);
    // int samplesize_N;
    vector<double> Lambda=Rcpp::as< std::vector<double> >(lambda);
    //int p=Gtmat.nrow();
    int n=Gtmat.ncol();// len of delta/omgea
    int p__=Gtmat.nrow();
    double tmp=0.;
    NumericVector S(n);
    //NumericVector uncenloc(floor(sum(Delta)));
    int cenlocL=n-floor(sum(Delta));
    NumericVector cenloc(cenlocL);
    int intflg=0;
    for (int i=0;i<n;i++){
        if (Delta(i)<.5) {
            cenloc(intflg)=i;//==0
            intflg++;
        }
    }
    Delta(n-1)=1;//setting the last to be observed!
    //start iteration
    NumericVector uomega(n);
    uomega(0)=1./(((double)n)-sum(Lambda,Gtmat,0));
    double Scen=0.;
    for (int k=1;k<n;k++){
        if (Delta(k)>.5){//==1
            tmp=0.;
            for (int i=0;i<n;i++){
                tmp+=uomega(i);
                S(i)=1-tmp;
            }
            Scen=0;
            if (cenloc(0)<(k-1)) {
                intflg=0;
                while (intflg<cenlocL  && cenloc(intflg)<=(k-1) ){
                    //if (k>47990) printf("Check point: %d iteration %dINTFLG\n",k,intflg);
                    Scen+=1./S(floor(cenloc(intflg)));
                    intflg++;
                }
            }
            uomega(k)=1./((double)n-sum(Lambda,Gtmat,k) -Scen);
        }
    }
    List re;
     /*
    NumericVector b__(n);
    for(int i=0;i<n;i++) b__[i]=delta[i]*uomega[i];
    */
    NumericVector chk(p__,0.);//or std::vector
    for (int i=0;i<p__;i++){
        for (int j=0;j<n;j++)
            chk(i)=chk(i)+ Delta(j)*uomega(j)*Gtmat(i,j);
    }
    NumericVector Gamma(n);
    for (int i=0;i<n;i++) Gamma(i)=1/S(i);
    re("omega")=uomega;re("gamma")=Gamma;re("S")=S;; re("chk")=chk;
    return(re);
}

extern "C"{
    void nocopy_kmc_data(int * delta,double * gtmat, 
                         double *uomega, int *np, double * chk){
        //chk[p]
        
        //gtmat \in \mathcal{R}^{p \times n}
        
        int nn=np[1];  // col: sample size
        //int p__=np[0]; // row: number of constraints.
        //double tmp=0.;

        vector<double>S(nn);
        int cenlocL=nn;
        for (int i=0;i<nn;++i) cenlocL -= delta[i];
        vector<int> cenloc(cenlocL); // Index of Censoring
        int intflg=0;
        for (int i=0;i<nn;i++){//TODO: Record index of cen
            if (delta[i]==0) {
                cenloc[intflg]=i;//==0
                intflg++;
            }
        }
        //Delta(n-1)=1;//setting the last to be observed!
        //start iteration
        /*
        vector<double>  lambda_gt(nn);
        for (size_t k=0;k<nn;k++){
            lambda_gt[k] = 0.;
            for (size_t kk = 0; kk < p__; kk++)
                lambda_gt[k] += lam[kk] * gtmat[nn * kk + k];
        }
        */

        uomega[0]=1./(((double)nn)- gtmat[0]);//iteration
        S[0] = 1. - uomega[0];
        double Scen=0.;
        int k = 0;
        while (k<nn-1){
            if (delta[k] ==0) Scen += 1/S[k];

            uomega[k+1] =  (delta[k+1]==1 ? 1./((double) nn - gtmat[k+1] - Scen):0.);
            S[k+1] = S[k] - uomega[k+1];
            /*if (delta[k]==1){//==1
                tmp=0.;
                for (int i=0;i<nn;i++){
                    tmp+=uomega[i];
                    S[i]=1-tmp;
                }
                Scen=0;
                if (cenloc[0]<(k-1)) {// notice starts from 1 as we computed k=0 manully
                    intflg=0;
                    while (intflg<cenlocL  && cenloc[intflg]<=(k-1) ){
                        //if (k>47990) printf("Check point: %d iteration %dINTFLG\n",k,intflg);
                        Scen+=1./S[cenloc[intflg]];
                        intflg++;
                    }
                }
                uomega[k]=1./((double)nn-sum(lam,gtmat,k,p__) -Scen);
            }
            */
           k++;
        }

        uomega[nn-1] = ( uomega[nn-1] < 0 ? 0:uomega[nn-1]);

    }
    
}


extern "C"{
    /**
     * @param delta: double array of length n;
     * @param lambda_gt: double array of length, ith entry = <lambda , g(T_i)> 
     *        Here <.,.> is the inner product;
     * @param w: jumps length = sum(delata)
     * @param np: {n, p}  
     * @test TBD
    */
    void kmc_native(double * delta, double * lambda_gt, double * w, int *np){
        int n = np[1];
        std::vector<double> S(n);
        size_t i = 0;
        double n_double = (double) n;
        for (size_t j =0;j < n; j++) w[j] = 0.;
        w[i] = 1/(n_double - lambda_gt[i]);
        double sumSjDelta0 = 0;
        S[i] = 1. -  w[i];
        while (i < n - 1){
            // Iterative Computing: KMC
            if (delta[i] < 0.5) {
            sumSjDelta0 += 1/S[i];
            }
            w[i+1] =  (delta[i+1]>0.0? 1./(n_double - lambda_gt[i+1] - sumSjDelta0):0.);
            S[i+1] = S[i] - w[i+1];
            i++;
        }
        #ifdef S_ROUTINE_
        //TODO: 
        w[n-1] = 1.;
        for (size_t j=0; j< n-1; j++) w[n-1] -=  w[j];
        #endif
        w[n-1] = ( w[n-1] < 0 ? 0:w[n-1]);
    }
}

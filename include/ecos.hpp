#pragma once

#include <Eigen/Sparse>

struct Settings
{
    const double gamma = 0.99;         // scaling the final step length
    const double delta = 2e-7;         // regularization parameter
    const double eps = 1e13;           // regularization threshold
    const double feastol = 1e-8;       // primal/dual infeasibility tolerance
    const double abstol = 1e-8;        // absolute tolerance on duality gap
    const double reltol = 1e-8;        // relative tolerance on duality gap
    const double feastol_inacc = 1e-4; // primal/dual infeasibility relaxed tolerance
    const double abstol_inacc = 5e-5;  // absolute relaxed tolerance on duality gap
    const double reltol_inacc = 5e-5;  // relative relaxed tolerance on duality gap
    const size_t nitref = 9;           // number of iterative refinement steps
    const size_t maxit = 100;          // maximum number of iterations
    const bool verbose = true;         // verbosity bool for PRINTLEVEL < 3
    const double linsysacc = 1e-14;    // rel. accuracy of search direction
    const double irerrfact = 6;        // factor by which IR should reduce err
    const double stepmin = 1e-6;       // smallest step that we do take
    const double stepmax = 0.999;      // largest step allowed, also in affine dir.
    const double sigmamin = 1e-4;      // always do some centering
    const double sigmamax = 1.;        // never fully center
    const size_t equil_iters = 3;
};

struct Information
{
    double pcost;
    double dcost;
    double pres;
    double dres;
    double pinf;
    double dinf;
    std::optional<double> pinfres;
    std::optional<double> dinfres;
    double gap;
    double relgap;
    double sigma;
    double mu;
    double step;
    double step_aff;
    double kapovert;
    size_t iter;
    size_t iter_max;
    size_t nitref1;
    size_t nitref2;
    size_t nitref3;
};

struct PositiveCone
{
    size_t dim;
    Eigen::VectorXd w;
    Eigen::VectorXd v;
    Eigen::VectorXi kkt_idx;
};

struct SecondOrderCone
{
    size_t dim;            // dimension of cone
    Eigen::VectorXd skbar; // temporary variables to work with
    Eigen::VectorXd zkbar; // temporary variables to work with
    double a;              // = wbar(1)
    double d1;             // first element of D
    double w;              // = q'*q
    double eta;            // eta = (sres / zres)^(1/4)
    double eta_square;     // eta^2 = (sres / zres)^(1/2)
    Eigen::VectorXd q;     // = wbar(2:end)
    Eigen::VectorXi Didx;  // indices for D
    double u0;             // eta
    double u1;             // u = [u0; u1 * q]
    double v1;             // v = [0; v1 * q]
};

class ECOSEigen
{
    // n:       Number of variables.
    // m:       Number of inequality constraints.
    // p:       Number of equality constraints.
    // l:       The dimension of the positive orthant, i.e. in Gx+s=h, s in K.
    // The first l elements of s are >=0, ncones is the number of second-order cones present in K.
    // ncones:  Number of second order cones in K.
    // q:       Vector of dimesions of each cone constraint in K.

    // A(p,n):  Equality constraint matrix.
    // b(p):    Equality constraint vector.
    // G(m,n):  Generalized inequality matrix.
    // h(m):    Generalized inequality vector.
    // c(n):    Variable weights.

    ECOSEigen(const Eigen::SparseMatrix<double> &G,
              const Eigen::SparseMatrix<double> &A,
              const Eigen::VectorXd &c,
              const Eigen::VectorXd &h,
              const Eigen::VectorXd &b,
              const std::vector<size_t> &soc_dims);

    void Solve();

private:
    PositiveCone lp_cone;
    std::vector<SecondOrderCone> so_cones;
    Settings settings;
    Information info, best_info;

    size_t iteration;

    Eigen::SparseMatrix<double> G;
    Eigen::SparseMatrix<double> A;
    Eigen::SparseMatrix<double> At;
    Eigen::SparseMatrix<double> Gt;
    Eigen::VectorXd c;
    Eigen::VectorXd h;
    Eigen::VectorXd b;

    Eigen::VectorXd x;      // Primal variables                     size num_var
    Eigen::VectorXd y;      // Multipliers for equality constaints  size num_eq
    Eigen::VectorXd z;      // Multipliers for conic inequalities   size num_ineq
    Eigen::VectorXd s;      // Slacks for conic inequalities        size num_ineq
    Eigen::VectorXd lambda; // Scaled variable                      size num_ineq

    // Residuals
    Eigen::VectorXd rx, ry, rz; // sizes num_var, num_eq, num_ineq
    double hresx, hresy, hresz;
    double rt;

    // Norm iterates
    double nx, ny, nz, ns;

    Eigen::VectorXd x_equil; // Equilibration vector of size n
    Eigen::VectorXd A_equil; // Equilibration vector of size num_eq
    Eigen::VectorXd G_equil; // Equilibration vector of size num_ineq

    size_t num_var;  // Number of variables (n)
    size_t num_eq;   // Number of equality constraints (p)
    size_t num_ineq; // Number of inequality constraints (m)
    size_t num_pc;   // Number of positive constraints (l)
    size_t num_sc;   // Number of second order cone constraints (ncones)
    size_t dim_K;    // Dimension of KKT matrix
    size_t D;        // Degree of the cone

    Eigen::VectorXd rhs1, rhs2; // The two right hand sides in the KKT equations.

    // Homogeneous embedding
    double kap; // kappa
    double tau; // tau

    // The problem data scaling parameters
    double scale_rx, scale_ry, scale_rz;
    double resx0, resy0, resz0;
    double cx, by, hz;

    Eigen::VectorXd dsaff_by_W, W_times_dzaff, dsaff;

    // KKT Matrix
    Eigen::SparseMatrix<double> K;
    using LDLT_t = Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>, Eigen::Upper>;
    LDLT_t ldlt;

    void setupKKT(const Eigen::SparseMatrix<double> &G,
                  const Eigen::SparseMatrix<double> &A);
    void updateKKT();
    void solveKKT(const Eigen::VectorXd &rhs,
                  Eigen::VectorXd &dx,
                  Eigen::VectorXd &dy,
                  Eigen::VectorXd &dz,
                  bool initialize);
    void bringToCone(Eigen::VectorXd &x);
    void computeResiduals();
    void updateStatistics();
    bool checkExitConditions(bool reduced_accuracy);
    bool updateScalings(const Eigen::VectorXd &s,
                        const Eigen::VectorXd &z,
                        Eigen::VectorXd &lambda);
    void RHS_affine();
    void RHS_combined();
    void scale2add(const Eigen::VectorXd &x, Eigen::VectorXd &y);
    void scale(const Eigen::VectorXd &z, Eigen::VectorXd &lambda);
    double lineSearch(Eigen::VectorXd &lambda,
                      Eigen::VectorXd &ds,
                      Eigen::VectorXd &dz,
                      double tau,
                      double dtau,
                      double kap,
                      double dkap);
    void conicProduct(const Eigen::VectorXd &u,
                      const Eigen::VectorXd &v,
                      Eigen::VectorXd &w);
    void conicDivision(const Eigen::VectorXd &u,
                       const Eigen::VectorXd &w,
                       Eigen::VectorXd &v);
    void backscale();
    void setEquilibration();
    void unsetEquilibration();
};
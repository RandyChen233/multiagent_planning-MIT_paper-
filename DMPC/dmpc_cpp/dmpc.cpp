//
// Created by carlos on 16/03/18.
//

#include <iostream>
#include "dmpc.h"

using namespace Eigen;
using namespace std;

SoftDMPC::SoftDMPC():
        _h(0.2),
        _T(20),
        _K(0),
        _k_hor(15),
        _order(2),
        _c(1.5),
        _rmin(0.5),
        _alim(0.5)
{
    _K =  _T/_h + 1; // number of time steps

    // Ellipsoid definitions
    Vector3d v(3);
    v << 1, 1, _c;
    _E = v.asDiagonal();
    _E1 = _E.inverse();
    _E2 = _E1.array().pow(_order);

    // Double integrator model
    _A << 1,0,0,_h,0,0,
          0,1,0,0,_h,0,
          0,0,1,0,0,_h,
          0,0,0,1,0,0,
          0,0,0,0,1,0,
          0,0,0,0,0,1;
    double h2 = pow(_h,2);
    _b << h2/2,0,0,
          0,h2/2,0,
          0,0,h2/2,
          _h,0,0,
          0,_h,0,
          0,0,_h;

    // Get matrices for MPC computation
    _Lambda = this->get_lambda_mat(_h,_k_hor);
    _Delta = this->get_delta_mat(_k_hor);
    _A0 = this->get_A0_mat(_k_hor);
    MatrixXd po = MatrixXd::Zero(3,1);
    MatrixXd pf = MatrixXd::Ones(3,1);

    Trajectory bgsrtg = this->init_dmpc(po,pf);
}

MatrixXd SoftDMPC::get_lambda_mat(int h, int K)
{
    MatrixXd Apos = MatrixXd::Zero(3*K,3*K);
    MatrixXd prev_row =  MatrixXd::Zero(6,3*K);
    MatrixXd new_row =  MatrixXd::Zero(6,3*K);
    MatrixXd add_b = MatrixXd::Zero(6,3*K);
    int idx = 0;
    for(int k=0; k < K; ++k)
    {
        add_b << MatrixXd::Zero(_b.rows(),_b.cols()*(k)),_b,
                MatrixXd::Zero(_b.rows(),_b.cols()*(K-k-1));
        new_row = _A*prev_row + add_b;
        Apos.middleRows(idx,3) = new_row.middleRows(0,3);
        prev_row = new_row;
        idx += 3;
    }
    return Apos;
}

MatrixXd SoftDMPC::get_delta_mat(int K)
{
    MatrixXd Delta = MatrixXd::Zero(3*K,3*K);
    MatrixXd new_row = MatrixXd::Zero(3,3*K);
    Delta.topRows(3) << MatrixXd::Identity(3,3),
                        MatrixXd::Zero(3,3*(K-1));
    MatrixXd b = MatrixXd::Zero(3,6);
    b << (-1)*MatrixXd::Identity(3,3), MatrixXd::Identity(3,3);
    int idx = 3;
    for(int k=0; k<K-1; ++k)
    {
       new_row <<  MatrixXd::Zero(3,3*k),b,MatrixXd::Zero(3,3*(K-k-2));
       Delta.middleRows(idx,3) = new_row;
       idx += 3;
    }
    return Delta;
}

MatrixXd SoftDMPC::get_A0_mat(int K)
{
    MatrixXd A0 = MatrixXd::Zero(3*K,6);
    MatrixXd new_row = MatrixXd::Zero(6,6);
    MatrixXd prev_row = MatrixXd::Identity(6,6);
    int idx = 0;
    for(int k=0; k<K; ++k)
    {
        new_row = _A*prev_row;
        A0.middleRows(idx,3) = new_row.middleRows(0,3);
        prev_row = new_row;
        idx += 3;
    }
    return A0;
}

Trajectory SoftDMPC::init_dmpc(MatrixXd po, MatrixXd pf)
{
    Trajectory init;
    MatrixXd diff = pf - po;
    VectorXd t = VectorXd::LinSpaced(_K,0,(_K-1)*_h);
    init.pos = MatrixXd::Zero(3,t.size());
    init.vel = MatrixXd::Zero(3,t.size());
    init.acc = MatrixXd::Zero(3,t.size());
    for (int i=0; i<t.size(); ++i)
    {
        init.pos.col(i) = po + t[i]*diff/((_K-1)*_h);
        cout << "init_pos = " << init.pos.col(i) << endl;
    }

    return init;
}





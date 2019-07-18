///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (C) 2018-2019, LAAS-CNRS
// Copyright note valid unless otherwise stated in individual files.
// All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef CROCODDYL_CORE_ACTIONS_DIFF_ACTION_LQR_HPP_
#define CROCODDYL_CORE_ACTIONS_DIFF_ACTION_LQR_HPP_

#include "crocoddyl/core/diff-action-base.hpp"
#include "crocoddyl/core/states/state-euclidean.hpp"

namespace crocoddyl {

struct DifferentialActionDataLQR;  // forward declaration

class DifferentialActionModelLQR : public DifferentialActionModelAbstract {
 public:
  DifferentialActionModelLQR(unsigned const int& nq, unsigned const int& nu, bool drift_free = true);
  ~DifferentialActionModelLQR();

  void calc(std::shared_ptr<DifferentialActionDataAbstract>& data, const Eigen::Ref<const Eigen::VectorXd>& x,
            const Eigen::Ref<const Eigen::VectorXd>& u) override;
  void calcDiff(std::shared_ptr<DifferentialActionDataAbstract>& data, const Eigen::Ref<const Eigen::VectorXd>& x,
                const Eigen::Ref<const Eigen::VectorXd>& u, const bool& recalc = true) override;
  std::shared_ptr<DifferentialActionDataAbstract> createData() override;

  Eigen::MatrixXd Fq_;
  Eigen::MatrixXd Fv_;
  Eigen::MatrixXd Fu_;
  Eigen::VectorXd f0_;
  Eigen::MatrixXd Lxx_;
  Eigen::MatrixXd Lxu_;
  Eigen::MatrixXd Luu_;
  Eigen::VectorXd lx_;
  Eigen::VectorXd lu_;

 private:
  bool drift_free_;
};

struct DifferentialActionDataLQR : public DifferentialActionDataAbstract {
  template <typename Model>
  DifferentialActionDataLQR(Model* const model) : DifferentialActionDataAbstract(model) {
    // Setting the linear model and quadratic cost here because they are constant
    const unsigned int& nq = model->get_nq();
    const unsigned int& nv = model->get_nv();
    Fx.topRows(nq) = model->Fq_;
    Fx.bottomRows(nv) = model->Fv_;
    Fu = model->Fu_;
    Lxx = model->Lxx_;
    Luu = model->Luu_;
    Lxu = model->Lxu_;
  }
  ~DifferentialActionDataLQR() {}
};

}  // namespace crocoddyl

#endif  // CROCODDYL_CORE_ACTIONS_DIFF_ACTION_LQR_HPP_
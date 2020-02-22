///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (C) 2018-2020, LAAS-CNRS, University of Edinburgh
// Copyright note valid unless otherwise stated in individual files.
// All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef CROCODDYL_CORE_ACTION_BASE_HPP_
#define CROCODDYL_CORE_ACTION_BASE_HPP_

#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "crocoddyl/core/fwd.hpp"
#include "crocoddyl/core/state-base.hpp"
#include "crocoddyl/core/utils/math.hpp"
#include "crocoddyl/core/utils/to-string.hpp"

namespace crocoddyl {

template <typename _Scalar>
class ActionModelAbstractTpl {
 public:
  typedef _Scalar Scalar;
  typedef ActionDataAbstractTpl<Scalar> ActionDataAbstract;
  typedef MathBaseTpl<Scalar> MathBase;

  ActionModelAbstractTpl(boost::shared_ptr<StateAbstractTpl<Scalar> > state, const std::size_t& nu,
                         const std::size_t& nr = 0);

  virtual ~ActionModelAbstractTpl();

  virtual void calc(const boost::shared_ptr<ActionDataAbstract>& data,
                    const Eigen::Ref<const typename MathBase::VectorXs>& x,
                    const Eigen::Ref<const typename MathBase::VectorXs>& u) = 0;
  virtual void calcDiff(const boost::shared_ptr<ActionDataAbstract>& data,
                        const Eigen::Ref<const typename MathBase::VectorXs>& x,
                        const Eigen::Ref<const typename MathBase::VectorXs>& u) = 0;
  virtual boost::shared_ptr<ActionDataAbstract> createData();

  void calc(const boost::shared_ptr<ActionDataAbstract>& data, const Eigen::Ref<const typename MathBase::VectorXs>& x);
  void calcDiff(const boost::shared_ptr<ActionDataAbstract>& data,
                const Eigen::Ref<const typename MathBase::VectorXs>& x);

  void quasiStatic(const boost::shared_ptr<ActionDataAbstract>& data, Eigen::Ref<typename MathBase::VectorXs> u,
                   const Eigen::Ref<const typename MathBase::VectorXs>& x, const std::size_t& maxiter = 100,
                   const Scalar& tol = 1e-9);

  const std::size_t& get_nu() const;
  const std::size_t& get_nr() const;
  const boost::shared_ptr<StateAbstractTpl<Scalar> >& get_state() const;

  const typename MathBase::VectorXs& get_u_lb() const;
  const typename MathBase::VectorXs& get_u_ub() const;
  bool const& get_has_control_limits() const;

  void set_u_lb(const typename MathBase::VectorXs& u_lb);
  void set_u_ub(const typename MathBase::VectorXs& u_ub);

 protected:
  std::size_t nu_;                                      //!< Control dimension
  std::size_t nr_;                                      //!< Dimension of the cost residual
  boost::shared_ptr<StateAbstractTpl<Scalar> > state_;  //!< Model of the state
  typename MathBase::VectorXs unone_;                   //!< Neutral state
  typename MathBase::VectorXs u_lb_;                    //!< Lower control limits
  typename MathBase::VectorXs u_ub_;                    //!< Upper control limits
  bool has_control_limits_;                             //!< Indicates whether any of the control limits is finite

  void update_has_control_limits();

#ifdef PYTHON_BINDINGS

 public:
  void calc_wrap(const boost::shared_ptr<ActionDataAbstract>& data, const typename MathBase::VectorXs& x,
                 const typename MathBase::VectorXs& u = typename MathBase::VectorXs()) {
    if (u.size() == 0) {
      calc(data, x);
    } else {
      calc(data, x, u);
    }
  }

  void calcDiff_wrap(const boost::shared_ptr<ActionDataAbstract>& data, const typename MathBase::VectorXs& x,
                     const typename MathBase::VectorXs& u) {
    calcDiff(data, x, u);
  }
  void calcDiff_wrap(const boost::shared_ptr<ActionDataAbstract>& data, const typename MathBase::VectorXs& x) {
    calcDiff(data, x, unone_);
  }

  typename MathBase::VectorXs quasiStatic_wrap(const boost::shared_ptr<ActionDataAbstract>& data,
                                               const typename MathBase::VectorXs& x, const std::size_t& maxiter = 100,
                                               const Scalar& tol = 1e-9) {
    typename MathBase::VectorXs u(nu_);
    u.setZero();
    quasiStatic(data, u, x, maxiter, tol);
    return u;
  }

#endif
};
template <typename _Scalar>
struct ActionDataAbstractTpl {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  typedef _Scalar Scalar;
  typedef MathBaseTpl<Scalar> MathBase;

  template <template <typename Scalar> class Model>
  explicit ActionDataAbstractTpl(Model<Scalar>* const model)
      : cost(0.),
        xnext(model->get_state()->get_nx()),
        r(model->get_nr()),
        Fx(model->get_state()->get_ndx(), model->get_state()->get_ndx()),
        Fu(model->get_state()->get_ndx(), model->get_nu()),
        Lx(model->get_state()->get_ndx()),
        Lu(model->get_nu()),
        Lxx(model->get_state()->get_ndx(), model->get_state()->get_ndx()),
        Lxu(model->get_state()->get_ndx(), model->get_nu()),
        Luu(model->get_nu(), model->get_nu()) {
    xnext.setZero();
    r.setZero();
    Fx.setZero();
    Fu.setZero();
    Lx.setZero();
    Lu.setZero();
    Lxx.setZero();
    Lxu.setZero();
    Luu.setZero();
  }
  virtual ~ActionDataAbstractTpl() {}

  Scalar cost;
  typename MathBase::VectorXs xnext;
  typename MathBase::VectorXs r;
  typename MathBase::MatrixXs Fx;
  typename MathBase::MatrixXs Fu;
  typename MathBase::VectorXs Lx;
  typename MathBase::VectorXs Lu;
  typename MathBase::MatrixXs Lxx;
  typename MathBase::MatrixXs Lxu;
  typename MathBase::MatrixXs Luu;
};

}  // namespace crocoddyl

/* --- Details -------------------------------------------------------------- */
/* --- Details -------------------------------------------------------------- */
/* --- Details -------------------------------------------------------------- */
#include "crocoddyl/core/action-base.hxx"

#endif  // CROCODDYL_CORE_ACTION_BASE_HPP_

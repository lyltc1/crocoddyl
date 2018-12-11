import abc
import numpy as np
from cddp.utils import EPS


class DynamicsData(object):
  """ Basic data structure for the system dynamics.

  We consider a general system dynamics as: d/dt([q; v]) = [v; a(q,v,u)]
  where q is the configuration point (R^{nq}), v is its tangent velocity (R^nv),
  u is the control vector (R^{nu}) and the system's state is x = [q; v]. The
  a() describes the acceleration evolution of the system. Note that in general
  q could be described with a number of tuples higher than nv (i.e. nq >= nv).
  For instance, if q is a point in a SE(3) manifold then we need 12 tuples (or 7
  tuples for quaternion-based description) to describe it.
  """
  __metaclass__ = abc.ABCMeta

  @abc.abstractmethod
  def __init__(self, dynamicModel, t, dt):
    """ Create the common dynamics data.

    :param dynamicModel: dynamics model
    :param t: initial time of the interval
    :param dt: step time of the interval
    """
    # Duration and initial time of the interval
    self.t = t
    self.dt = dt

    # System acceleration
    self.a = np.zeros((dynamicModel.nv(), 1))

    # Terms for linear approximation, which has the form:
    #   d/dt([q; v]) = [0, I; aq, av]*[q; v] + [0; au]*u
    self.aq = np.zeros((dynamicModel.nv(), dynamicModel.nv()))
    self.av = np.zeros((dynamicModel.nv(), dynamicModel.nv()))
    self.au = np.zeros((dynamicModel.nv(), dynamicModel.nu()))

    # Creating the discretizer data
    if dt != 0.:
      self.discretizer = dynamicModel.discretizer.createData(dynamicModel, dt)

    self.diff_x = np.zeros((dynamicModel.nx(), 1))

    # TODO: think if we need to create NumDiff data
    self.h = np.sqrt(EPS)
    self.x_pert = np.zeros((dynamicModel.nx(), 1))
    self.q_pert = np.zeros((dynamicModel.nq(), 1))
    self.v_pert = np.zeros((dynamicModel.nv(), 1))
    self.u_pert = np.zeros((dynamicModel.nu(), 1))


class DynamicsModel(object):
  """ This abstract class declares virtual methods for updating the dynamics
  and its linear approximation.

  We consider a general system dynamic as: d/dt([q; v]) = [v; a(q,v,u)]
  where q is the configuration point (R^{nq}), v is its tangent velocity (R^nv),
  u is the control vector (R^{nu}) and the system's state is x = [q; v]. The
  a() describes the acceleration evolution of the system. Note that in general
  q could be described with a number of tuples higher than nv (i.e. nq >= nv).
  For instance, if q is a point in a SE(3) manifold then we need 12 tuples (or 7
  tuples for quaternion-based description) to describe it.
  """
  __metaclass__ = abc.ABCMeta

  def __init__(self, integrator, discretizer, nq, nv, nu):
    """ Create the dynamic model.

    :param integrator: system integrator
    :param discretizer: system discretizer
    :param nq: number of tuples that describe the configuration point
    :param nv: dimension of the configuration space
    :param nu: dimension of control vector
    """
    self.integrator = integrator
    self.discretizer = discretizer
    self._nq = nq
    self._nv = nv
    self._nu = nu
    # Computing the dimension of the state space
    self._nx_impl = nq + nv
    self._nx = 2 * nv

  @abc.abstractmethod
  def createData(self, t, dt):
    """ Create the dynamics data.

    :param t: starting time
    :param dt: step integration
    """
    raise NotImplementedError("Not implemented yet.")

  @abc.abstractmethod
  def updateTerms(self, dynamicsData, x):
    """ Update the terms needed for an user-defined dynamics.

    :param dynamicsData: dynamics data
    :param x: state
    :param u: control
    """
    raise NotImplementedError("Not implemented yet.")

  @abc.abstractmethod
  def updateDynamics(self, dynamicsData, x, u):
    """ Update the user-defined dynamics.

    :param dynamicsData: dynamics data
    :param x: state
    :param u: control
    """
    raise NotImplementedError("Not implemented yet.")

  @abc.abstractmethod
  def updateLinearAppr(self, dynamicsData, x, u):
    """ Update the user-defined dynamics.

    :param dynamicsData: dynamics data
    :param x: state
    :param u: control
    """
    # Resetting the acceleration derivatives
    dynamicsData.aq.fill(0.)
    dynamicsData.av.fill(0.)
    dynamicsData.au.fill(0.)

    # Computing the dynamics by a perturbation in the configuration
    dynamicsData.x_pert[self.nq():] = x[self.nq():]
    for i in xrange(self.nv()):
      # Compute the pertubation in the configuration
      dynamicsData.v_pert.fill(0.)
      dynamicsData.v_pert[i] += dynamicsData.h
      np.copyto(dynamicsData.q_pert,
        self.integrateConfiguration(x[:self.nq()], dynamicsData.v_pert))
      dynamicsData.x_pert[:self.nq()] = dynamicsData.q_pert

      # Update the dynamics given a perturbation in the configuration and sum up
      self.updateDynamics(dynamicsData, dynamicsData.x_pert, u)

      # Summing up this perturbation
      dynamicsData.aq[:,i] += np.array(dynamicsData.a)[:,0]


    # Computing the dynamics by a perturbation in the velocity
    np.copyto(dynamicsData.x_pert, x)
    for i in xrange(self.nv()):
      # Compute the pertubation in the velocity
      dynamicsData.x_pert[self.nq():] = x[self.nq():]
      dynamicsData.x_pert[i+self.nv()] += dynamicsData.h

      # Update the dynamics given a perturbation in the velocity and sum up
      self.updateDynamics(dynamicsData, dynamicsData.x_pert, u)
      dynamicsData.av[:,i] += np.array(dynamicsData.a)[:,0]

    # Computing the dynamics by a perturbation in the control
    for i in xrange(self.nu()):
      # Compute the pertubation in the control
      np.copyto(dynamicsData.u_pert, u)
      dynamicsData.u_pert[i] += dynamicsData.h

      # Update the dynamics given a perturbation in the control and sum up
      self.updateDynamics(dynamicsData, x, dynamicsData.u_pert)
      dynamicsData.au[:,i] += np.array(dynamicsData.a)[:,0]

    # Adding the nominal acceleration
    self.updateDynamics(dynamicsData, x, u)
    dynamicsData.aq -= dynamicsData.a
    dynamicsData.av -= dynamicsData.a
    dynamicsData.au -= dynamicsData.a

    # Getting the final derivatives (i.e. da/dq, da/dv, da/du). For that we
    # divide by delta_t
    dynamicsData.av /= dynamicsData.h
    dynamicsData.aq /= dynamicsData.h
    dynamicsData.au /= dynamicsData.h

  @abc.abstractmethod
  def integrateConfiguration(self, q, dq):
    """ Operator that integrates the configuration.

    :param q: current configuration point
    :param dq: displacement of the configuration
    """
    raise NotImplementedError("Not implemented yet.")

  @abc.abstractmethod
  def differenceConfiguration(self, q0, q1):
    """ Operator that differentiates the configuration.

    :param q0: current configuration point
    :param q1: next configurtion point
    """
    raise NotImplementedError("Not implemented yet.")

  def forwardRunningCalc(self, dynamicsData, x, u, xNext):
    # Updating the dynamics
    self.updateDynamics(dynamicsData, x, u)

    # Integrating the dynanics
    self.integrator(self, dynamicsData, x, u, xNext)

  def forwardTerminalCalc(self, dynamicsData, x):
    # Updating the dynamic terms
    self.updateTerms(dynamicsData, x)

  def backwardRunningCalc(self, dynamicsData, x, u):
    # Updating the continuous-time linear approximation
    self.updateLinearAppr(dynamicsData, x, u)
    # Discretizing this linear approximation
    self.discretizer(self, dynamicsData)

  def backwardTerminalCalc(self, dynamicsData, x):
    # Updating the dynamic terms
    self.updateTerms(dynamicsData, x)

  def differenceState(self, dynamicsData, x0, x1):
    dynamicsData.diff_x[self.nv():] = \
        x1[self.nq():,:] - x0[self.nq():,:]
    dynamicsData.diff_x[:self.nv()] = \
        self.differenceConfiguration(x0[:self.nq()], x1[:self.nq()])
    return dynamicsData.diff_x

  def nq(self):
    """ Return the number of tuples used to describe the configuration point.
    """
    return self._nq

  def nv(self):
    """ Return the dimension of the configuration space.
    """
    return self._nv

  def nu(self):
    """ Return the dimension of the control vector.
    """
    return self._nu

  def nxImpl(self):
    """ Return the number of tuples used to describe the state.
    """
    return self._nx_impl

  def nx(self):
    """ Return the dimension of the state space.
    """
    return self._nx
/* Copyright (c) 2012-2017 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */

#ifndef Cell_CPP
#define Cell_CPP

#include "Cell.hpp"

namespace Kiva {

static const double PI = 4.0*atan(1.0);


Cell::Cell(const std::size_t &index, const CellType cellType,
           const std::size_t &i, const std::size_t &j, const std::size_t &k,
           const Foundation &foundation, Surface *surfacePtr, Block *blockPtr,
           Mesher *meshXptr, Mesher *meshYptr, Mesher *meshZptr):
        i(i), j(j), k(k),
        index(index),
        cellType(cellType),
        blockPtr(blockPtr),
        surfacePtr(surfacePtr),
        meshXptr(meshXptr),
        meshYptr(meshYptr),
        meshZptr(meshZptr)
{
  if (blockPtr) {
    density = blockPtr->material.density;
    specificHeat = blockPtr->material.specificHeat;
    conductivity = blockPtr->material.conductivity;
  } else {
    density = foundation.soil.density;
    specificHeat = foundation.soil.specificHeat;
    conductivity = foundation.soil.conductivity;
  }
  heatGain = 0.0;
  volume = meshXptr->deltas[i]*meshYptr->deltas[j]*meshZptr->deltas[k];

  // for boundary cells, set cell area
  if (cellType == CellType::BOUNDARY)
  {
    if (foundation.numberOfDimensions == 2 &&
        foundation.coordinateSystem == Foundation::CS_CYLINDRICAL)
    {
      if (surfacePtr->orientation == Surface::X_POS ||
          surfacePtr->orientation == Surface::X_NEG)
      {
        area = 2.0*PI*meshXptr->centers[i]*meshZptr->deltas[k];
      }
      else // if (surface.orientation == Surface::Z_POS ||
        // surface.orientation == Surface::Z_NEG)
      {
        area = PI*(meshXptr->dividers[i+1]*meshXptr->dividers[i+1] -
                   meshXptr->dividers[i]*meshXptr->dividers[i] );
      }
    }
    else if (foundation.numberOfDimensions == 2 &&
             foundation.coordinateSystem == Foundation::CS_CARTESIAN)
    {
      if (surfacePtr->orientation == Surface::X_POS ||
          surfacePtr->orientation == Surface::X_NEG)
      {
        area = 2.0*meshZptr->deltas[k]*foundation.linearAreaMultiplier;
      }
      else // if (surface.orientation == Surface::Z_POS ||
        // surface.orientation == Surface::Z_NEG)
      {
        area = 2.0*meshXptr->deltas[i]*foundation.linearAreaMultiplier;
      }
    }
    else if (foundation.numberOfDimensions == 3)
    {
      if (surfacePtr->orientation == Surface::X_POS ||
          surfacePtr->orientation == Surface::X_NEG)
      {
        area = meshYptr->deltas[j]*meshZptr->deltas[k];
      }
      else if (surfacePtr->orientation == Surface::Y_POS ||
               surfacePtr->orientation == Surface::Y_NEG)
      {
        area = meshXptr->deltas[i]*meshZptr->deltas[k];
      }
      else // if (surface.orientation == Surface::Z_POS ||
        // surface.orientation == Surface::Z_NEG)
      {
        area = meshXptr->deltas[i]*meshYptr->deltas[j];
      }

      if (foundation.useSymmetry)
      {
        if (foundation.isXSymm)
          area = 2*area;

        if (foundation.isYSymm)
          area = 2*area;
      }
    }
    else
    {
      area = 1.0;
    }
  }

  if (cellType == CellType::ZERO_THICKNESS & foundation.numberOfDimensions == 2) {
    if (i != 0)
      r = meshXptr->centers[i];
  }
}

void Cell::setNeighbors(std::vector<Cell> &cell_v, std::size_t stepsize_i,
                        std::size_t stepsize_j, std::size_t stepsize_k)
{
  i_up_Ptr = &cell_v[index+stepsize_i];
  i_down_Ptr = &cell_v[index-stepsize_i];
  j_up_Ptr = &cell_v[index+stepsize_j];
  j_down_Ptr = &cell_v[index-stepsize_j];
  k_up_Ptr = &cell_v[index+stepsize_k];
  k_down_Ptr = &cell_v[index-stepsize_k];
}

void Cell::setDistances(double &dxp_in, double &dxm_in, double &dyp_in, double &dym_in,
                        double &dzp_in, double &dzm_in) {
  dxp = dxp_in;
  dxm = dxm_in;
  dyp = dyp_in;
  dym = dym_in;
  dzp = dzp_in;
  dzm = dzm_in;
}

void Cell::setConductivities() {
  getKXP();
  getKXM();
  getKYP();
  getKYM();
  getKZP();
  getKZM();
}

void Cell::getKXP()
{
  if (i == meshXptr->centers.size() - 1)
  {
    // For boundary cells assume that the cell on the other side of the
    // boundary is the same as the current cell
    kxp = conductivity;
  }
  else
  {
    kxp = 1/(meshXptr->deltas[i]/(2*dxp*conductivity) +
             meshXptr->deltas[i + 1]/(2*dxp*i_up_Ptr->conductivity));
  }
}

void Cell::getKXM()
{
  if (i == 0)
  {
    // For boundary cells assume that the cell on the other side of the
    // boundary is the same as the current cell
    kxm = conductivity;
  }
  else
  {
    kxm = 1/(meshXptr->deltas[i]/(2*dxm*conductivity) +
             meshXptr->deltas[i - 1]/(2*dxm*i_down_Ptr->conductivity));
  }
}

void Cell::getKYP()
{
  if (j == meshYptr->centers.size() - 1)
  {
    // For boundary cells assume that the cell on the other side of the
    // boundary is the same as the current cell
    kyp = conductivity;
  }
  else
  {
    kyp = 1/(meshYptr->deltas[j]/(2*dyp*conductivity) +
             meshYptr->deltas[j + 1]/(2*dyp*j_up_Ptr->conductivity));
  }
}

void Cell::getKYM()
{
  if (j == 0)
  {
    // For boundary cells assume that the cell on the other side of the
    // boundary is the same as the current cell
    kym = conductivity;
  }
  else
  {
    kym = 1/(meshYptr->deltas[j]/(2*dym*conductivity) +
             meshYptr->deltas[j - 1]/(2*dym*j_down_Ptr->conductivity));
  }
}

void Cell::getKZP()
{
  if (k == meshZptr->centers.size() - 1)
  {
    // For boundary cells assume that the cell on the other side of the
    // boundary is the same as the current cell
    kzp = conductivity;
  }
  else
  {
    kzp = 1/(meshZptr->deltas[k]/(2*dzp*conductivity) +
             meshZptr->deltas[k + 1]/(2*dzp*k_up_Ptr->conductivity));
  }
}

void Cell::getKZM()
{
  if (k == 0)
  {
    // For boundary cells assume that the cell on the other side of the
    // boundary is the same as the current cell
    kzm = conductivity;
  }
  else
  {
    kzm = 1/(meshZptr->deltas[k]/(2*dzm*conductivity) +
             meshZptr->deltas[k - 1]/(2*dzm*k_down_Ptr->conductivity));
  }
}

void Cell::setWhatever(int ndims, bool cylindrical) {
  if (ndims > 1) {
    // Radial X terms
    if (cylindrical)
    {
      cxp_c = (dxm*kxp) / ((dxm + dxp)*dxp);
      cxm_c = (dxp*kxm) / ((dxm + dxp)*dxm);
    }
    else
    {
      cxp_c = 0.0;
      cxm_c = 0.0;
    }

    // Cartesian X terms
    cxp = (2*kxp) /  ((dxm + dxp)*dxp);
    cxm = -1*(2*kxm) / ((dxm + dxp)*dxm);
  }

  // Cartesian Z terms
  czp = (2*kzp) / ((dzm + dzp)*dzp);
  czm = -1*(2*kzm) / ((dzm + dzp)*dzm);

  // Cartesian Y terms
  if (ndims == 3)
  {
    cyp = (2*kyp) / ((dym + dyp)*dyp);
    cym = -1*(2*kym) / ((dym + dyp)*dym);
  }
  else
  {
    cyp = 0.0;
    cym = 0.0;
  }
}

void Cell::setZeroThicknessCellProperties(std::vector<Cell*> pointSet)
{
  std::vector<double> volumes;
  std::vector<double> densities;
  std::vector<double> specificHeats;
  std::vector<double> conductivities;

  std::vector<double> masses;
  std::vector<double> capacities;
  std::vector<double> weightedConductivity;

  for (auto p_cell : pointSet)
  {
    // Do not add air cell properties into the weighted average
    if (p_cell->cellType != CellType::INTERIOR_AIR &&
        p_cell->cellType != CellType::EXTERIOR_AIR)
    {
      double vol = p_cell->volume;
      double rho = p_cell->density;
      double cp = p_cell->specificHeat;
      double kth = p_cell->conductivity;

      volumes.push_back(vol);
      masses.push_back(vol*rho);
      capacities.push_back(vol*rho*cp);
      weightedConductivity.push_back(vol*kth);
    }
  }

  // if the neighboring cells are all air cells set properties to air properties
  if (volumes.size() == 0)
  {
    volumes.push_back(1.0);
    masses.push_back(1.275);
    capacities.push_back(1.275*1007);
    weightedConductivity.push_back(0.02587);

  }

  double totalVolume = std::accumulate(volumes.begin(), volumes.end(), 0.0);

  density = std::accumulate(masses.begin(), masses.end(), 0.0) /
            totalVolume;

  specificHeat = std::accumulate(capacities.begin(), capacities.end(), 0.0) /
                 (totalVolume*density);

  conductivity = std::accumulate(weightedConductivity.begin(), weightedConductivity.end(), 0.0) /
                 totalVolume;
}



void Cell::calcCellADEUp(double timestep, const Foundation &foundation,
                         std::vector<double> &U, std::vector<double> &UOld)
{
  double theta = timestep/
                 (density*specificHeat);

  double CXP = cxp*theta;
  double CXM = cxm*theta;
  double CZP = czp*theta;
  double CZM = czm*theta;
  double CYP = cyp*theta;
  double CYM = cym*theta;
  double Q = heatGain*theta;

  if (foundation.numberOfDimensions == 3)
    U[index] = (UOld[index]*(1.0 - CXP - CZP - CYP)
                - U[i_down_Ptr->index]*CXM
                + UOld[i_up_Ptr->index]*CXP
                - U[k_down_Ptr->index]*CZM
                + UOld[k_up_Ptr->index]*CZP
                - U[j_down_Ptr->index]*CYM
                + UOld[j_up_Ptr->index]*CYP
                + Q) /
               (1.0 - CXM - CZM - CYM);
  else if (foundation.numberOfDimensions == 2)
  {
    double CXPC = 0;
    double CXMC = 0;

    if (i != 0)
    {
      CXPC = cxp_c*theta/r;
      CXMC = cxm_c*theta/r;
    }
    U[index] = (UOld[index]*(1.0 - CXPC - CXP - CZP)
                - U[i_down_Ptr->index]*(CXMC + CXM)
                + UOld[i_up_Ptr->index]*(CXPC + CXP)
                - U[k_down_Ptr->index]*CZM
                + UOld[k_up_Ptr->index]*CZP
                + Q) /
               (1.0 - CXMC - CXM - CZM);
  }
  else
  {
    U[index] = (UOld[index]*(1.0 - CZP)
                - U[k_down_Ptr->index]*CZM
                + UOld[k_up_Ptr->index]*CZP
                + Q) /
               (1.0 - CZM);
  }
}

void Cell::calcCellADEDown(double timestep, const Foundation &foundation,
                           std::vector<double> &V, std::vector<double> &VOld)
{
  double theta = timestep/
                 (density*specificHeat);

  double CXP = cxp*theta;
  double CXM = cxm*theta;
  double CZP = czp*theta;
  double CZM = czm*theta;
  double Q = heatGain*theta;

  if (foundation.numberOfDimensions == 3) {
    double CYP = cyp * theta;
    double CYM = cym * theta;
    V[index] = (VOld[index] * (1.0 + CXM + CZM + CYM)
                - VOld[i_down_Ptr->index] * CXM
                + V[i_up_Ptr->index] * CXP
                - VOld[k_down_Ptr->index] * CZM
                + V[k_up_Ptr->index] * CZP
                - VOld[j_down_Ptr->index] * CYM
                + V[j_up_Ptr->index] * CYP
                + Q) /
               (1.0 + CXP + CZP + CYP);
  } else if (foundation.numberOfDimensions == 2)
  {
    double CXPC = 0;
    double CXMC = 0;

    if (i != 0)
    {
      CXPC = cxp_c*theta/r;
      CXMC = cxm_c*theta/r;
    }
    V[index] = (VOld[index]*(1.0 + CXMC + CXM + CZM)
                - VOld[i_down_Ptr->index]*(CXMC + CXM)
                + V[i_up_Ptr->index]*(CXPC + CXP)
                - VOld[k_down_Ptr->index]*CZM
                + V[k_up_Ptr->index]*CZP
                + Q) /
               (1.0 + CXPC + CXP + CZP);
  }
  else
  {
    V[index] = (VOld[index]*(1.0 + CZM)
                - VOld[k_down_Ptr->index]*CZM
                + V[k_up_Ptr->index]*CZP
                + Q) /
               (1.0 + CZP);
  }
}

double Cell::calcCellExplicit(double timestep, const Foundation &foundation)
{
  double theta = timestep/
                 (density*specificHeat);

  double CXP = cxp*theta;
  double CXM = cxm*theta;
  double CZP = czp*theta;
  double CZM = czm*theta;
  double Q = heatGain*theta;

  if (foundation.numberOfDimensions == 3) {
    double CYP = cyp*theta;
    double CYM = cym*theta;
    double TNew = *told_ptr * (1.0 + CXM + CZM + CYM - CXP - CZP - CYP)
                  - *i_down_Ptr->told_ptr * CXM
                  + *i_up_Ptr->told_ptr * CXP
                  - *k_down_Ptr->told_ptr * CZM
                  + *k_up_Ptr->told_ptr * CZP
                  - *j_down_Ptr->told_ptr * CYM
                  + *j_up_Ptr->told_ptr * CYP
                  + Q;
    return TNew;
  }
  if (foundation.numberOfDimensions == 2)
  {
    double CXPC = 0;
    double CXMC = 0;

    if (i != 0)
    {
      CXPC = cxp_c*theta/r;
      CXMC = cxm_c*theta/r;
    }

    double TNew = *told_ptr*(1.0 + CXMC + CXM + CZM - CXPC - CXP - CZP)
                  - *i_down_Ptr->told_ptr*(CXMC + CXM)
                  + *i_up_Ptr->told_ptr*(CXPC + CXP)
                  - *k_down_Ptr->told_ptr*CZM
                  + *k_up_Ptr->told_ptr*CZP
                  + Q;
    return TNew;
  }

  double TNew = *told_ptr*(1.0 + CZM - CZP)
                - *k_down_Ptr->told_ptr*CZM
                + *k_up_Ptr->told_ptr*CZP
                + Q;
  return TNew;
}

void Cell::calcCellMatrix(Foundation::NumericalScheme scheme, double timestep, const Foundation &foundation,
        std::vector<Eigen::Triplet<double>> &tripletList, Eigen::VectorXd &b,
        std::vector<double> &a1, std::vector<double> &a2, std::vector<double> &a3, std::vector<double> &b_)
{
  if (scheme == Foundation::NS_STEADY_STATE)
  {
    calcCellSteadyState(foundation, tripletList, b, a1, a2, a3, b_);
  }
  else
  {
    double theta = timestep/
                   (density*specificHeat);

    double f;
    if (scheme == Foundation::NS_IMPLICIT)
      f = 1.0;
    else
      f = 0.5;

    double CXP = cxp*theta;
    double CXM = cxm*theta;
    double CZP = czp*theta;
    double CZM = czm*theta;
    double CYP = cyp*theta;
    double CYM = cym*theta;
    double Q = heatGain*theta;

    if (foundation.numberOfDimensions == 3)
    {
      double A = (1.0 + f*(CXP + CZP + CYP - CXM - CZM - CYM));
      double Aim = f*CXM;
      double Aip = f*(-CXP);
      double Akm = f*CZM;
      double Akp = f*(-CZP);
      double Ajm = f*CYM;
      double Ajp = f*(-CYP);

      double bVal = *told_ptr*(1.0 + (1-f)*(CXM + CZM + CYM - CXP - CZP - CYP))
             - *i_down_Ptr->told_ptr*(1-f)*CXM
             + *i_up_Ptr->told_ptr*(1-f)*CXP
             - *j_down_Ptr->told_ptr*(1-f)*CZM
             + *j_up_Ptr->told_ptr*(1-f)*CZP
             - *k_down_Ptr->told_ptr*(1-f)*CYM
             + *k_up_Ptr->told_ptr*(1-f)*CYP
             + Q;

      tripletList.emplace_back(index,index,A);
      tripletList.emplace_back(index,i_up_Ptr->index,Aip);
      tripletList.emplace_back(index,i_down_Ptr->index,Aim);
      tripletList.emplace_back(index,j_up_Ptr->index,Ajp);
      tripletList.emplace_back(index,j_down_Ptr->index,Ajm);
      tripletList.emplace_back(index,k_up_Ptr->index,Akp);
      tripletList.emplace_back(index,k_down_Ptr->index,Akm);
      b(index) = bVal;
    }
    else if (foundation.numberOfDimensions == 2)
    {
      double CXPC = 0;
      double CXMC = 0;

      if (i != 0)
      {
        CXPC = cxp_c*theta/r;
        CXMC = cxm_c*theta/r;
      }
      double A = (1.0 + f*(CXPC + CXP + CZP - CXMC - CXM - CZM));
      double Aim = f*(CXMC + CXM);
      double Aip = f*(-CXPC - CXP);
      double Akm = f*CZM;
      double Akp = f*(-CZP);

      double bVal = *told_ptr*(1.0 + (1-f)*(CXMC + CXM + CZM - CXPC - CXP - CZP))
             - *i_down_Ptr->told_ptr*(1-f)*(CXMC + CXM)
             + *i_up_Ptr->told_ptr*(1-f)*(CXPC + CXP)
             - *k_down_Ptr->told_ptr*(1-f)*CZM
             + *k_up_Ptr->told_ptr*(1-f)*CZP
             + Q;

      tripletList.emplace_back(index,index,A);
      tripletList.emplace_back(index,i_up_Ptr->index,Aip);
      tripletList.emplace_back(index,i_down_Ptr->index,Aim);
      tripletList.emplace_back(index,k_up_Ptr->index,Akp);
      tripletList.emplace_back(index,k_down_Ptr->index,Akm);
      b(index) = bVal;
    }
    else
    {
      double A = (1.0 + f*(CZP - CZM));
      double Akm = f*CZM;
      double Akp = f*(-CZP);

      double bVal = *told_ptr*(1.0 + (1-f)*(CZM - CZP))
             - *k_down_Ptr->told_ptr*(1-f)*CZM
             + *k_up_Ptr->told_ptr*(1-f)*CZP
             + Q;

      a2[index] = A;
      a3[index] = Akp;
      a1[index] = Akm;
      b_[index] = bVal;
    }
  }

}

void Cell::calcCellSteadyState(const Foundation &foundation,
                               std::vector<Eigen::Triplet<double>> &tripletList, Eigen::VectorXd &b,
                               std::vector<double> &a1, std::vector<double> &a2, std::vector<double> &a3,
                               std::vector<double> &b_)
{
  double CXP = cxp;
  double CXM = cxm;
  double CZP = czp;
  double CZM = czm;
  double CYP = cyp;
  double CYM = cym;
  double Q = heatGain;

  if (foundation.numberOfDimensions == 3)
  {
    double A = (CXM + CZM + CYM - CXP - CZP - CYP);
    double Aim = -CXM;
    double Aip = CXP;
    double Akm = -CZM;
    double Akp = CZP;
    double Ajm = -CYM;
    double Ajp = CYP;

    double bVal = -Q;

    tripletList.emplace_back(index,index,A);
    tripletList.emplace_back(index,i_up_Ptr->index,Aip);
    tripletList.emplace_back(index,i_down_Ptr->index,Aim);
    tripletList.emplace_back(index,j_up_Ptr->index,Ajp);
    tripletList.emplace_back(index,j_down_Ptr->index,Ajm);
    tripletList.emplace_back(index,k_up_Ptr->index,Akp);
    tripletList.emplace_back(index,k_down_Ptr->index,Akm);
    b(index) = bVal;
  }
  else if (foundation.numberOfDimensions == 2)
  {
    double CXPC = 0;
    double CXMC = 0;

    if (i != 0)
    {
      CXPC = cxp_c/r;
      CXMC = cxm_c/r;
    }
    double A = (CXMC + CXM + CZM - CXPC - CXP - CZP);
    double Aim = (-CXMC - CXM);
    double Aip = (CXPC + CXP);
    double Akm = -CZM;
    double Akp = CZP;

    double bVal = -Q;

    tripletList.emplace_back(index,index,A);
    tripletList.emplace_back(index,i_up_Ptr->index,Aip);
    tripletList.emplace_back(index,i_down_Ptr->index,Aim);
    tripletList.emplace_back(index,k_up_Ptr->index,Akp);
    tripletList.emplace_back(index,k_down_Ptr->index,Akm);
    b(index) = bVal;
  }
  else
  {
    double A = (CZM - CZP);
    double Akm = -CZM;
    double Akp = CZP;

    double bVal = -Q;

    a2[index] = A;
    a3[index] = Akp;
    a1[index] = Akm;
    b_[index] = bVal;
  }
}

void Cell::calcCellADI(int dim, const Foundation &foundation, double timestep,
                       double &Am, double &A, double &Ap, double &bVal) {
  double theta = timestep / (foundation.numberOfDimensions
                             *density*specificHeat);

  double CXP = cxp*theta;
  double CXM = cxm*theta;
  double CZP = czp*theta;
  double CZM = czm*theta;
  double CYP = cyp*theta;
  double CYM = cym*theta;
  double Q = heatGain*theta;

  double f = foundation.fADI;

  if (foundation.numberOfDimensions == 3)
  {
    if (dim == 1) // x
    {
      A = 1.0 + (3 - 2*f)*(CXP - CXM);
      Am = (3 - 2*f)*CXM;
      Ap = (3 - 2*f)*(-CXP);

      bVal = *told_ptr*(1.0 + f*(CZM + CYM - CZP - CYP))
             - *k_down_Ptr->told_ptr*f*CZM
             + *k_up_Ptr->told_ptr*f*CZP
             - *j_down_Ptr->told_ptr*f*CYM
             + *j_up_Ptr->told_ptr*f*CYP
             + Q;
    }
    else if (dim == 2) // y
    {
      A = (1.0 + (3 - 2*f)*(CYP - CYM));
      Am = (3 - 2*f)*CYM;
      Ap = (3 - 2*f)*(-CYP);

      bVal = *told_ptr*(1.0 + f*(CXM + CZM - CXP - CZP))
             - *i_down_Ptr->told_ptr*f*CXM
             + *i_up_Ptr->told_ptr*f*CXP
             - *k_down_Ptr->told_ptr*f*CZM
             + *k_up_Ptr->told_ptr*f*CZP
             + Q;
    }
    else //if (dim == 3) // z
    {
      A = (1.0 + (3 - 2*f)*(CZP - CZM));
      Am = (3 - 2*f)*CZM;
      Ap = (3 - 2*f)*(-CZP);

      bVal = *told_ptr*(1.0 + f*(CXM + CYM - CXP - CYP))
             - *i_down_Ptr->told_ptr*f*CXM
             + *i_up_Ptr->told_ptr*f*CXP
             - *j_down_Ptr->told_ptr*f*CYM
             + *j_up_Ptr->told_ptr*f*CYP
             + Q;
    }

  }
  else if (foundation.numberOfDimensions == 2)
  {
    double CXPC = 0;
    double CXMC = 0;
    if (i != 0)
    {
      CXPC = cxp_c*theta/r;
      CXMC = cxm_c*theta/r;
    }
    if (dim == 1) // x
    {
      A = 1.0 + (2 - f)*(CXPC + CXP - CXMC - CXM);
      Am = (2 - f)*(CXMC + CXM);
      Ap = (2 - f)*(-CXPC - CXP);

      bVal = *told_ptr*(1.0 + f*(CZM - CZP))
             - *k_down_Ptr->told_ptr*f*CZM
             + *k_up_Ptr->told_ptr*f*CZP
             + Q;
    }
    else //if (dim == 3) // z
    {
      A = 1.0 + (2 - f)*(CZP - CZM);
      Am = (2 - f)*CZM;
      Ap = (2 - f)*(-CZP);

      bVal = *told_ptr*(1.0 + f*(CXMC + CXM - CXPC - CXP))
             - *i_down_Ptr->told_ptr*f*(CXMC + CXM)
             + *i_up_Ptr->told_ptr*f*(CXPC + CXP)
             + Q;
    }
  }
  else
  {
    A = 1.0 + CZP - CZM;
    Am = CZM;
    Ap = -CZP;

    bVal = *told_ptr + Q;
  }
}


}

#endif

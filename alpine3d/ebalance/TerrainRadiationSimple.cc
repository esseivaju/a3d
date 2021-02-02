/***********************************************************************************/
/*  Copyright 2009-2015 WSL Institute for Snow and Avalanche Research    SLF-DAVOS      */
/***********************************************************************************/
/* This file is part of Alpine3D.
    Alpine3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Alpine3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Alpine3D.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <alpine3d/ebalance/TerrainRadiationSimple.h>
#include <alpine3d/ebalance/ViewFactors.h>

using namespace mio;

TerrainRadiationSimple::TerrainRadiationSimple(const mio::Config& i_cfg, const mio::DEMObject& dem_in, const std::string& method)
            : TerrainRadiationAlgorithm(method,dem_in.getNx(), dem_in.getNy()),
            alb_spatial_mean(dem_in.getNx(), dem_in.getNy(), IOUtils::nodata),
            sky_vf(dem_in.getNx(), dem_in.getNy(), IOUtils::nodata),
            dimx(dem_in.getNx()), dimy(dem_in.getNy()), startx(0), endx(dimx)
{
	// In case we're running with MPI enabled, calculate the slice this process is responsible for
	size_t nx = dimx;
	MPIControl::instance().getArraySliceParams(dimx, startx, nx);
	endx = startx + nx;

	initSkyViewFactor(dem_in);

}

TerrainRadiationSimple::~TerrainRadiationSimple() {}

void TerrainRadiationSimple::getRadiation(const mio::Array2D<double>& direct,
                                          mio::Array2D<double>& diffuse, mio::Array2D<double>& terrain,
                                          mio::Array2D<double>& direct_unshaded_horizontal,
                                          double solarAzimuth, double solarElevation)
{
	MPIControl& mpicontrol = MPIControl::instance();
	terrain.resize(dimx, dimy, 0.);  //so allreduce_sum works properly when it sums full grids
	Array2D<double> diff_corr(dimx, dimy, 0.); //so allreduce_sum works properly when it sums full grids

	if (mpicontrol.master()) {
		std::cout << "[i] Calculating terrain radiation with simple method, using " << mpicontrol.size();
		if (mpicontrol.size()>1) std::cout << " processes\n";
		else std::cout << " process\n";
	}

	#pragma omp parallel for collapse(2)
	for (size_t jj=0; jj<dimy; jj++) {
		for (size_t ii=startx; ii<endx; ii++) {
			if (sky_vf(ii,jj) == IOUtils::nodata) {
				terrain(ii,jj) = IOUtils::nodata;
				diff_corr(ii,jj) = IOUtils::nodata;
			} else {
				const double terrain_reflected = alb_spatial_mean(ii,jj) * (direct_unshaded_horizontal(ii,jj)+diffuse(ii,jj));
				const double terrain_viewFactor = 1. - sky_vf(ii,jj);
				terrain(ii,jj) = terrain_viewFactor * terrain_reflected;
				diff_corr(ii,jj) = diffuse(ii,jj) * sky_vf(ii,jj);
			}
		}
	}

	mpicontrol.allreduce_sum(terrain);
	mpicontrol.allreduce_sum(diff_corr);
	diffuse = diff_corr; //return the corrected diffuse radiation
}


void TerrainRadiationSimple::setMeteo(const mio::Array2D<double>& albedo,
                                      const mio::Array2D<double>& alb_spatial_mean_in,
                                      const mio::Array2D<double>& /*ta*/, const mio::Array2D<double>& /*rh*/,
                                      const mio::Array2D<double>& /*ilwr*/)
{
	albedo_grid = albedo;
  alb_spatial_mean=alb_spatial_mean_in;
}

void TerrainRadiationSimple::getSkyViewFactor(mio::Array2D<double> &o_sky_vf) {
	o_sky_vf = sky_vf;
	MPIControl::instance().allreduce_sum(o_sky_vf);
}

void TerrainRadiationSimple::initSkyViewFactor(const mio::DEMObject &dem)
{
	#pragma omp parallel for collapse(2)
	for (size_t jj=0; jj<dimy; jj++) {
		for (size_t ii=startx; ii<endx; ii++) {
			if (dem(ii,jj) != IOUtils::nodata)
				sky_vf(ii,jj) = mio::DEMAlgorithms::getCellSkyViewFactor(dem, ii, jj);
		}
	}
}

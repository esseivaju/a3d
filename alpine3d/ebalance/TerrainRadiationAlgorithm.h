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
#ifndef TERRAINRADIATIONALGORITHM_H
#define TERRAINRADIATIONALGORITHM_H

#include <meteoio/MeteoIO.h>
#include <alpine3d/ebalance/RadiationField.h>
#include <alpine3d/ebalance/SolarPanel.h>

//Class for shooting cells
class CellsList {
	public:
		double radiation;
		int x;
		int y;
};

inline bool operator_greater(const CellsList& a, const CellsList& b) {
	return a.radiation > b.radiation;
}

class TerrainRadiationAlgorithm {
	public:
		TerrainRadiationAlgorithm(const std::string& i_algo, size_t nx, size_t ny)
                             : algo(i_algo), albedo_grid(nx, ny, mio::IOUtils::nodata), _hasSP(false) {}		virtual ~TerrainRadiationAlgorithm();
		const bool hasSP(){return _hasSP;}
		virtual void getRadiation(const mio::Array2D<double>& direct, mio::Array2D<double>& diffuse,
                              mio::Array2D<double>& terrain, mio::Array2D<double>& direct_unshaded_horizontal,
                              mio::Array2D<double>& view_factor, double solarAzimuth, double solarElevation) = 0;
		virtual void setMeteo(const mio::Array2D<double>& albedo, const mio::Array2D<double>& alb_spatial_mean,
                          const mio::Array2D<double>& ta, const mio::Array2D<double>& rh,
                          const mio::Array2D<double>& ilwr) = 0;
		const std::string algo;
		void setSP(const mio::Date timestamp, const double solarAzimuth, const double solarElevation){};
		void writeSP(const unsigned int max_steps){};

	protected:
		mio::Array2D<double> albedo_grid;
		bool _hasSP;
};

class TerrainRadiationFactory {
	public:
		// FELIX: const RadiationField* radfield
		static TerrainRadiationAlgorithm* getAlgorithm(const mio::Config& cfg, const mio::DEMObject &dem, const int& nbworkers);
};

#endif

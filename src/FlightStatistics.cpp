/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "FlightStatistics.hpp"

void FlightStatistics::Reset() {
  ScopeLock lock(mutex);

  thermal_average.Reset();
  altitude.Reset();
  altitude_base.Reset();
  altitude_ceiling.Reset();
  task_speed.Reset();
  altitude_terrain.Reset();
}

void
FlightStatistics::StartTask()
{
  ScopeLock lock(mutex);
  // JMW clear thermal climb average on task start
  thermal_average.Reset();
  task_speed.Reset();
}

void
FlightStatistics::AddAltitudeTerrain(const double tflight, const double terrainalt)
{
  ScopeLock lock(mutex);
  altitude_terrain.Update(std::max(0., tflight / 3600.),
                          terrainalt);
}

void
FlightStatistics::AddAltitude(const double tflight, const double alt)
{
  ScopeLock lock(mutex);
  altitude.Update(std::max(0., tflight / 3600.), alt);
}

double
FlightStatistics::AverageThermalAdjusted(const double mc_current,
                                         const bool circling)
{
  ScopeLock lock(mutex);

  double mc_stats;
  if (thermal_average.GetAverageY() > 0) {
    if (mc_current > 0 && circling)
      mc_stats = (thermal_average.GetCount() * thermal_average.GetAverageY() + mc_current) /
        (thermal_average.GetCount() + 1);
    else
      mc_stats = thermal_average.GetAverageY();
  } else {
    mc_stats = mc_current;
  }

  return mc_stats;
}

void
FlightStatistics::AddTaskSpeed(const double tflight, const double val)
{
  ScopeLock lock(mutex);
  task_speed.Update(tflight / 3600, std::max(0., val));
}

void
FlightStatistics::AddClimbBase(const double tflight, const double alt)
{
  ScopeLock lock(mutex);

  if (!altitude_ceiling.IsEmpty())
    // only update base if have already climbed, otherwise
    // we will catch the takeoff height as the base.
    altitude_base.Update(std::max(0., tflight) / 3600., alt);
}

void
FlightStatistics::AddClimbCeiling(const double tflight, const double alt)
{
  ScopeLock lock(mutex);
  altitude_ceiling.Update(std::max(0., tflight) / 3600., alt);
}

/**
 * Adds a thermal to the ThermalAverage calculator
 * @param v Average climb speed of the last thermal
 */
void
FlightStatistics::AddThermalAverage(const double v)
{
  ScopeLock lock(mutex);
  thermal_average.Update(v);
}

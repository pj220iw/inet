//
// Copyright (C) 2014 OpenSim Ltd.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef __INET_GRIDNEIGHBORCACHE_H
#define __INET_GRIDNEIGHBORCACHE_H

#include "inet/common/geometry/container/SpatialGrid.h"
#include "inet/physicallayer/wireless/common/medium/RadioMedium.h"

namespace inet {
namespace physicallayer {

class INET_API GridNeighborCache : public cSimpleModule, public INeighborCache
{
  public:
    typedef std::vector<const IRadio *> Radios;

  protected:
    class GridNeighborCacheVisitor : public IVisitor {
      protected:
        RadioMedium *radioMedium;
        IRadio *transmitter;
        const IWirelessSignal *signal;

      public:
        void visit(const cObject *radio) const override;
        GridNeighborCacheVisitor(RadioMedium *radioMedium, IRadio *transmitter, const IWirelessSignal *signal) :
            radioMedium(radioMedium), transmitter(transmitter), signal(signal) {}
    };

  protected:
    SpatialGrid *grid;
    Radios radios;
    RadioMedium *radioMedium;
    Coord constraintAreaMin, constraintAreaMax;
    cMessage *refillCellsTimer;
    double refillPeriod;
    double maxSpeed;
    Coord cellSize;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    void fillCubeVector();

  public:
    GridNeighborCache();
    virtual ~GridNeighborCache();

    virtual std::ostream& printToStream(std::ostream& stream, int level, int evFlags = 0) const override;

    virtual void addRadio(const IRadio *radio) override;
    virtual void removeRadio(const IRadio *radio) override;
    virtual void sendToNeighbors(IRadio *transmitter, const IWirelessSignal *signal, double range) const override;
};

} // namespace physicallayer
} // namespace inet

#endif


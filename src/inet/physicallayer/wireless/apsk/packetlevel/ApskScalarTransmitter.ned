//
// Copyright (C) 2013 OpenSim Ltd.
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

package inet.physicallayer.wireless.apsk.packetlevel;

import inet.physicallayer.wireless.common.base.packetlevel.NarrowbandTransmitterBase;

//
// This transmitter model produces transmissions that have scalar transmission
// power in their analog representation and the configured modulation.
//
// @see ~ApskScalarReceiver, ~ApskScalarRadio, ~ApskScalarRadioMedium.
//
module ApskScalarTransmitter extends NarrowbandTransmitterBase
{
    parameters:
        modulation = default("BPSK");
        @class(ApskScalarTransmitter);
}


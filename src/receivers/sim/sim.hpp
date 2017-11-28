/*
   sim.hpp : Support USB controller for flight simulators

   Controller subclasses Receiver

   This file is part of Hackflight.

   Hackflight is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Hackflight is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with Hackflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "receiver.hpp"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>

#include <board.hpp>

namespace hf {

    class Controller : public Receiver {

        public:

            bool useSerial(void)
            {
                return true;
            }

            Controller(void)
            {
                _reversedVerticals = false;
                _springyThrottle = false;
                _useButtonForAux = false;
                _joyid = 0;
            }

            void begin(void)
            {
                // Set up axes based on OS and controller
                productInit();

                // Useful for springy-throttle controllers (XBox, PS3)
                _throttleDemand = -1.f;
            }

            float readChannel(uint8_t chan)
            {
                static float demands[5];

                // Poll on first channel request
                if (chan == 0) {
                    poll(demands);
                }

                // Special handling for throttle
                return (chan == 0) ? _throttleDemand : demands[chan];
            }

            void halt(void)
            {
            }

        private:

            // implemented differently for each OS
            void     productInit(void);
            void     productPoll(int32_t axes[6]);
            int32_t  productGetBaseline(void);

            bool     _reversedVerticals;
            bool     _springyThrottle;
            bool     _useButtonForAux;
            float    _throttleDemand;
            uint8_t  _axismap[5];   // Thr, Ael, Ele, Rud, Aux
            int      _joyid;        // Linux file descriptor or Windows joystick ID

            void poll(float * demands)
            {
                static int32_t axes[6];

                // grab the axis values in an OS-specific way
                productPoll(axes);

                // normalize the axes to demands in [-1,+1]
                for (uint8_t k=0; k<5; ++k) {
                    demands[k] = (axes[_axismap[k]] - productGetBaseline()) / 32767.f;
                }

                // invert throttle, pitch if indicated
                if (_reversedVerticals) {
                    demands[0] = -demands[0];
                    demands[2] = -demands[2];
                }

                // for game controllers, use buttons for aux
                if (_useButtonForAux) {
                    demands[4] = -1; // XXX for now, disallow aux switch
                }

                // game-controller spring-mounted throttle requires special handling
                if (_springyThrottle) {
                    demands[0] = Filter::deadband(demands[0], 0.15);
                    _throttleDemand += demands[0] * .01f; // XXX need to make this deltaT computable
                    _throttleDemand = Filter::constrainAbs(_throttleDemand, 1);
                }
                else {
                    _throttleDemand = demands[0];
                }
            }

    }; // class Controller

} // namespace hf

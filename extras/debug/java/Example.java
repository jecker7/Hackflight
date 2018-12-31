/*
   Example for parsing MSPPG messages with Java

   Mocks up an ATTITUDE_RADIANS message response from the flight controller
   and uses the Parser object to parse the response bytes

   Copyright (C) Simon D. Levy 2018

   This code is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as 
   published by the Free Software Foundation, either version 3 of the 
   License, or (at your option) any later version.
   This code is distributed in the hope that it will be useful,     
   but WITHOUT ANY WARRANTY without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License 
   along with this code.  If not, see <http:#www.gnu.org/licenses/>.
 */

import edu.wlu.cs.msppg.*;

public class Example implements ATTITUDE_RADIANS_Handler {

    public void handle_ATTITUDE_RADIANS(float angx, float angy, float heading) {

        System.out.printf("%+3.3f %+3.3f %+3.3f\n", angx, angy, heading);
    }

    public static void main(String [] argv) {

        Parser parser = new Parser();

        byte [] buf = parser.serialize_ATTITUDE_RADIANS(1,2,3);

        /*
           Example handler = new Example();

           parser.set_ATTITUDE_RADIANS_Handler(handler);

           for (byte b : buf) {

           parser.parse(b);

           }*/
    }

}
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean, Meng Tan

*/

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestDiffTimeval
#include <boost/test/auto_unit_test.hpp>

#define LOGNULL
#include "log.hpp"
#include "difftimeval.hpp"

BOOST_AUTO_TEST_CASE(TestUsecToTimeval)
{
    uint64_t usec = 0L;
    timeval res;
    res = usectotimeval(usec);
    BOOST_CHECK_EQUAL(res.tv_sec,  0);
    BOOST_CHECK_EQUAL(res.tv_usec, 0);

    usec = 459327L;
    res = usectotimeval(usec);
    BOOST_CHECK_EQUAL(res.tv_sec,  0);
    BOOST_CHECK_EQUAL(res.tv_usec, 459327);

    usec = 5476000000LL;
    res = usectotimeval(usec);
    BOOST_CHECK_EQUAL(res.tv_sec,  5476);
    BOOST_CHECK_EQUAL(res.tv_usec, 0);

    usec = 751379115L;
    res = usectotimeval(usec);
    BOOST_CHECK_EQUAL(res.tv_sec,  751);
    BOOST_CHECK_EQUAL(res.tv_usec, 379115);

}

BOOST_AUTO_TEST_CASE(TestAddUsecTimeval)
{
    uint64_t usec = 0L;
    struct timeval init = { 0, 0 };
    timeval res;

    res = addusectimeval(usec, init);
    BOOST_CHECK_EQUAL(res.tv_sec,  0);
    BOOST_CHECK_EQUAL(res.tv_usec, 0);


    usec = 654322;
    res = addusectimeval(usec, init);
    BOOST_CHECK_EQUAL(res.tv_sec,  0);
    BOOST_CHECK_EQUAL(res.tv_usec, 654322);

    usec = 42737465L;
    res = addusectimeval(usec, init);
    BOOST_CHECK_EQUAL(res.tv_sec,  42);
    BOOST_CHECK_EQUAL(res.tv_usec, 737465);

    usec = 0;
    init.tv_sec =  78;
    init.tv_usec = 426891;
    res = addusectimeval(usec, init);
    BOOST_CHECK_EQUAL(res.tv_sec,  78);
    BOOST_CHECK_EQUAL(res.tv_usec, 426891);

    init.tv_sec =  78;
    init.tv_usec = 0;
    usec = 91234;
    res = addusectimeval(usec, init);
    BOOST_CHECK_EQUAL(res.tv_sec,  78);
    BOOST_CHECK_EQUAL(res.tv_usec, 91234);

    init.tv_sec =  815;
    init.tv_usec = 945862;
    usec = 457369421;
    res = addusectimeval(usec, init);
    BOOST_CHECK_EQUAL(res.tv_sec,  1273);
    BOOST_CHECK_EQUAL(res.tv_usec, 315283);


}

BOOST_AUTO_TEST_CASE(TestLessThanTimeVal)
{
    timeval before, after;
    before.tv_sec = 0;
    before.tv_usec = 0;
    after = before;

    BOOST_CHECK_EQUAL(lessthantimeval(before, after), false);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

    after.tv_sec += 1;
    BOOST_CHECK_EQUAL(lessthantimeval(before, after), true);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

    after = before;
    after.tv_usec += 1;
    BOOST_CHECK_EQUAL(lessthantimeval(before, after), true);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

    after = before;
    after.tv_sec += 1;
    after.tv_usec += 1;
    BOOST_CHECK_EQUAL(lessthantimeval(before, after), true);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);


    before.tv_sec = 184635;
    before.tv_usec = 564732;
    after = before;

    BOOST_CHECK_EQUAL(lessthantimeval(before, after), false);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

    after.tv_sec += 1;
    BOOST_CHECK_EQUAL(lessthantimeval(before, after), true);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

    after = before;
    after.tv_usec += 1;
    BOOST_CHECK_EQUAL(lessthantimeval(before, after), true);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

    after = before;
    after.tv_sec += 1;
    after.tv_usec += 1;
    BOOST_CHECK_EQUAL(lessthantimeval(before, after), true);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);


    after = before;

    BOOST_CHECK_EQUAL(lessthantimeval(before, after), false);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

    after.tv_sec += 4651;
    BOOST_CHECK_EQUAL(lessthantimeval(before, after), true);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

    after = before;
    after.tv_usec += 573;
    BOOST_CHECK_EQUAL(lessthantimeval(before, after), true);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

    after = before;
    after.tv_sec += 1272;
    after.tv_usec += 151236;
    BOOST_CHECK_EQUAL(lessthantimeval(before, after), true);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

    after = before;

    BOOST_CHECK_EQUAL(lessthantimeval(before, after), false);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

    before.tv_sec -= 1;
    BOOST_CHECK_EQUAL(lessthantimeval(before, after), true);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

    before = after;
    before.tv_usec -= 1;
    BOOST_CHECK_EQUAL(lessthantimeval(before, after), true);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

    before = after;
    before.tv_sec -= 1;
    before.tv_usec -= 1;
    BOOST_CHECK_EQUAL(lessthantimeval(before, after), true);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);


    before = after;

    BOOST_CHECK_EQUAL(lessthantimeval(before, after), false);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

    before.tv_sec -= 13477;
    BOOST_CHECK_EQUAL(lessthantimeval(before, after), true);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

    before = after;
    before.tv_usec -= 321697;
    BOOST_CHECK_EQUAL(lessthantimeval(before, after), true);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

    before = after;
    before.tv_sec -= 324;
    before.tv_usec -= 3277;
    BOOST_CHECK_EQUAL(lessthantimeval(before, after), true);
    BOOST_CHECK_EQUAL(lessthantimeval(after, before), false);

}


BOOST_AUTO_TEST_CASE(Testaddtimeval)
{
    timeval time1, time2;
    timeval res, sym;
    time1.tv_sec  = 0;
    time1.tv_usec = 0;
    time2 = time1;

    res = addtimeval(time1, time2);
    BOOST_CHECK_EQUAL(res.tv_sec, 0);
    BOOST_CHECK_EQUAL(res.tv_usec, 0);
    sym = addtimeval(time2, time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);

    time1.tv_usec = 486541;
    res = addtimeval(time1, time2);
    BOOST_CHECK_EQUAL(res.tv_sec, 0);
    BOOST_CHECK_EQUAL(res.tv_usec, 486541);
    sym = addtimeval(time2, time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);


    time1.tv_usec = 0;
    time1.tv_sec = 975;
    res = addtimeval(time1, time2);
    BOOST_CHECK_EQUAL(res.tv_sec, 975);
    BOOST_CHECK_EQUAL(res.tv_usec, 0);
    sym = addtimeval(time2, time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);


    time1.tv_sec = 1237;
    time1.tv_usec = 4873;
    res = addtimeval(time1, time2);
    BOOST_CHECK_EQUAL(res.tv_sec, 1237);
    BOOST_CHECK_EQUAL(res.tv_usec, 4873);
    sym = addtimeval(time2, time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);


    time1.tv_sec = 234321;
    time1.tv_usec = 354879;
    time2.tv_sec = 157532;
    time2.tv_usec = 245789;
    res = addtimeval(time1, time2);
    BOOST_CHECK_EQUAL(res.tv_sec, 391853);
    BOOST_CHECK_EQUAL(res.tv_usec, 600668);
    sym = addtimeval(time2, time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);

    time1.tv_sec = 236321;
    time1.tv_usec = 354879;
    time2.tv_sec = 157532;
    time2.tv_usec = 745789;
    res = addtimeval(time1, time2);
    BOOST_CHECK_EQUAL(res.tv_sec, 393854);
    BOOST_CHECK_EQUAL(res.tv_usec, 100668);
    sym = addtimeval(time2, time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);
}

BOOST_AUTO_TEST_CASE(Testabsdifftimeval)
{
    timeval time1, time2;
    timeval res, sym;

    time1.tv_sec  = 0;
    time1.tv_usec = 0;
    time2 = time1;
    res = absdifftimeval(time1, time2);
    BOOST_CHECK_EQUAL(res.tv_sec, 0);
    BOOST_CHECK_EQUAL(res.tv_usec, 0);
    sym = absdifftimeval(time2, time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);


    time1.tv_sec  = 3457;
    time1.tv_usec = 215733;
    res = absdifftimeval(time1, time2);
    BOOST_CHECK_EQUAL(res.tv_sec, 3457);
    BOOST_CHECK_EQUAL(res.tv_usec, 215733);
    sym = absdifftimeval(time2, time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);

    time2 = time1;
    res = absdifftimeval(time1, time2);
    BOOST_CHECK_EQUAL(res.tv_sec, 0);
    BOOST_CHECK_EQUAL(res.tv_usec, 0);
    sym = absdifftimeval(time2, time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);


    time2.tv_usec += 457352;
    res = absdifftimeval(time1, time2);
    BOOST_CHECK_EQUAL(res.tv_sec, 0);
    BOOST_CHECK_EQUAL(res.tv_usec, 457352);
    sym = absdifftimeval(time2, time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);

    time2.tv_sec += 54573;
    res = absdifftimeval(time1, time2);
    BOOST_CHECK_EQUAL(res.tv_sec, 54573);
    BOOST_CHECK_EQUAL(res.tv_usec, 457352);
    sym = absdifftimeval(time2, time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);


    time2.tv_usec = time1.tv_usec;
    res = absdifftimeval(time1, time2);
    BOOST_CHECK_EQUAL(res.tv_sec, 54573);
    BOOST_CHECK_EQUAL(res.tv_usec, 0);
    sym = absdifftimeval(time2, time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);

    time1.tv_sec  = 67235;
    time1.tv_usec = 454324;
    time2.tv_sec  = 421;
    time2.tv_usec = 842136;
    res = absdifftimeval(time1, time2);
    BOOST_CHECK_EQUAL(res.tv_sec, 66813);
    BOOST_CHECK_EQUAL(res.tv_usec, 612188);
    sym = absdifftimeval(time2, time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);


}

BOOST_AUTO_TEST_CASE(TestMultTimeval)
{
    timeval init, res;
    uint64_t mult;

    mult = 0;
    init.tv_sec  = 0;
    init.tv_usec = 0;
    res = multtimeval(mult, init);
    BOOST_CHECK_EQUAL(res.tv_sec, 0);
    BOOST_CHECK_EQUAL(res.tv_usec, 0);

    mult = 0;
    init.tv_sec  = 3235357;
    init.tv_usec = 243230;
    res = multtimeval(mult, init);
    BOOST_CHECK_EQUAL(res.tv_sec, 0);
    BOOST_CHECK_EQUAL(res.tv_usec, 0);

    mult = 43721099;
    init.tv_sec  = 0;
    init.tv_usec = 0;
    res = multtimeval(mult, init);
    BOOST_CHECK_EQUAL(res.tv_sec, 0);
    BOOST_CHECK_EQUAL(res.tv_usec, 0);

    mult = 1;
    init.tv_sec  = 234221;
    init.tv_usec = 1098;
    res = multtimeval(mult, init);
    BOOST_CHECK_EQUAL(res.tv_sec, 234221);
    BOOST_CHECK_EQUAL(res.tv_usec, 1098);

    mult = 12;
    init.tv_sec  = 24671;
    init.tv_usec = 11566;
    res = multtimeval(mult, init);
    BOOST_CHECK_EQUAL(res.tv_sec, 296052);
    BOOST_CHECK_EQUAL(res.tv_usec, 138792);

    mult = 33;
    init.tv_sec  = 7499;
    init.tv_usec = 111566;
    res = multtimeval(mult, init);
    BOOST_CHECK_EQUAL(res.tv_sec, 247470);
    BOOST_CHECK_EQUAL(res.tv_usec, 681678);

}

BOOST_AUTO_TEST_CASE(Testmintimeval)
{
    timeval time1, time2;
    timeval res, sym;

    time1.tv_sec  = 67235;
    time1.tv_usec = 454324;
    time2.tv_sec  = 421;
    time2.tv_usec = 842136;
    res = mintimeval(time1,time2);
    BOOST_CHECK_EQUAL(res.tv_sec, time2.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, time2.tv_usec);
    sym = mintimeval(time2,time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);


    time1.tv_sec  = 421;
    time1.tv_usec = 454324;
    time2.tv_sec  = 421;
    time2.tv_usec = 842136;
    res = mintimeval(time1,time2);
    BOOST_CHECK_EQUAL(res.tv_sec, time1.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, time1.tv_usec);
    sym = mintimeval(time2,time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);

    time1.tv_sec  = 246575;
    time1.tv_usec = 454324;
    time2.tv_sec  = 421;
    time2.tv_usec = 454324;
    res = mintimeval(time1,time2);
    BOOST_CHECK_EQUAL(res.tv_sec, time2.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, time2.tv_usec);
    sym = mintimeval(time2,time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);

    time1.tv_sec  = 246575;
    time1.tv_usec = 457324;
    time2.tv_sec  = 421;
    time2.tv_usec = 454324;
    res = mintimeval(time1,time2);
    BOOST_CHECK_EQUAL(res.tv_sec, time2.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, time2.tv_usec);
    sym = mintimeval(time2,time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);

    time2 = time1;
    res = mintimeval(time1,time2);
    BOOST_CHECK_EQUAL(res.tv_sec, time2.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, time2.tv_usec);
    BOOST_CHECK_EQUAL(res.tv_sec, time1.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, time1.tv_usec);
    sym = mintimeval(time2,time1);
    BOOST_CHECK_EQUAL(res.tv_sec, sym.tv_sec);
    BOOST_CHECK_EQUAL(res.tv_usec, sym.tv_usec);

}

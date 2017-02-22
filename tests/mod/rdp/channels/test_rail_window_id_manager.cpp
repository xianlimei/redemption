/*
    This program is free software; you can redistribute it and/or modify it
     under the terms of the GNU General Public License as published by the
     Free Software Foundation; either version 2 of the License, or (at your
     option) any later version.

    This program is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
     Public License for more details.

    You should have received a copy of the GNU General Public License along
     with this program; if not, write to the Free Software Foundation, Inc.,
     675 Mass Ave, Cambridge, MA 02139, USA.

    Product name: redemption, a FLOSS RDP proxy
    Copyright (C) Wallix 2015
    Author(s): Christophe Grosjean, Raphael Zhou
*/

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestCLIPRDRChannel
#include "system/redemption_unit_tests.hpp"

#define LOGNULL
//#define LOGPRINT

#include "mod/rdp/channels/rail_window_id_manager.hpp"

BOOST_AUTO_TEST_CASE(TestRailWindowIDManagerGeneral)
{
    RemoteProgramsWindowIdManager rail_window_id_manager;


    BOOST_CHECK_EQUAL(rail_window_id_manager.get_client_window_id_ex(40020), 40020);

    BOOST_CHECK_EQUAL(rail_window_id_manager.get_client_window_id(40020), 40020);
    BOOST_CHECK_EQUAL(rail_window_id_manager.get_server_window_id(40020), 40020);

    BOOST_CHECK_EQUAL(rail_window_id_manager.is_client_only_window(40020), false);


    uint32_t client_only_window_id = rail_window_id_manager.register_client_window();

    BOOST_CHECK_EQUAL(rail_window_id_manager.get_server_window_id(client_only_window_id),
        uint32_t(RemoteProgramsWindowIdManager::INVALID_WINDOW_ID));

    BOOST_CHECK_EQUAL(rail_window_id_manager.is_client_only_window(client_only_window_id), true);


    uint32_t server_window_id = client_only_window_id;
    uint32_t client_window_id = rail_window_id_manager.get_client_window_id_ex(server_window_id);

    BOOST_CHECK(server_window_id != client_window_id);

    BOOST_CHECK_EQUAL(rail_window_id_manager.get_client_window_id(server_window_id), client_window_id);
    BOOST_CHECK_EQUAL(rail_window_id_manager.get_server_window_id(client_window_id), server_window_id);

    BOOST_CHECK_EQUAL(rail_window_id_manager.is_client_only_window(client_window_id), false);
}

BOOST_AUTO_TEST_CASE(TestRailWindowIDManagerGeneralUnregisterClientWindow)
{
    RemoteProgramsWindowIdManager rail_window_id_manager;

    uint32_t client_only_window_id = rail_window_id_manager.register_client_window();

    BOOST_CHECK_EQUAL(rail_window_id_manager.get_server_window_id(client_only_window_id),
        uint32_t(RemoteProgramsWindowIdManager::INVALID_WINDOW_ID));

    rail_window_id_manager.unregister_client_only_window(client_only_window_id);
}

BOOST_AUTO_TEST_CASE(TestRailWindowIDManagerUnregisterMappedServerWindow0)
{
    RemoteProgramsWindowIdManager rail_window_id_manager;

    BOOST_CHECK_EQUAL(rail_window_id_manager.get_client_window_id_ex(40020), 40020);

    BOOST_CHECK_EQUAL(rail_window_id_manager.get_client_window_id(40020), 40020);
    BOOST_CHECK_EQUAL(rail_window_id_manager.get_server_window_id(40020), 40020);

    rail_window_id_manager.unregister_server_window(40020);
}

BOOST_AUTO_TEST_CASE(TestRailWindowIDManagerUnregisterMappedServerWindow1)
{
    RemoteProgramsWindowIdManager rail_window_id_manager;

    BOOST_CHECK_EQUAL(rail_window_id_manager.get_client_window_id_ex(40020), 40020);

    BOOST_CHECK_EQUAL(rail_window_id_manager.get_client_window_id(40020), 40020);
    BOOST_CHECK_EQUAL(rail_window_id_manager.get_server_window_id(40020), 40020);

    rail_window_id_manager.unregister_server_window(40020);
}

BOOST_AUTO_TEST_CASE(TestRailWindowIDManagerUnregisterMappedServerWindow2)
{
    RemoteProgramsWindowIdManager rail_window_id_manager;


    uint32_t client_only_window_id = rail_window_id_manager.register_client_window();

    BOOST_CHECK_EQUAL(rail_window_id_manager.get_server_window_id(client_only_window_id),
        uint32_t(RemoteProgramsWindowIdManager::INVALID_WINDOW_ID));


    uint32_t server_window_id = client_only_window_id;
    uint32_t client_window_id = rail_window_id_manager.get_client_window_id_ex(server_window_id);

    BOOST_CHECK(server_window_id != client_window_id);

    BOOST_CHECK_EQUAL(rail_window_id_manager.get_client_window_id(server_window_id), client_window_id);
    BOOST_CHECK_EQUAL(rail_window_id_manager.get_server_window_id(client_window_id), server_window_id);


    rail_window_id_manager.unregister_server_window(server_window_id);
}

BOOST_AUTO_TEST_CASE(TestRailWindowIDManagerUnregisterMappedServerWindow3)
{
    RemoteProgramsWindowIdManager rail_window_id_manager;


    uint32_t client_only_window_id = rail_window_id_manager.register_client_window();

    BOOST_CHECK_EQUAL(rail_window_id_manager.get_server_window_id(client_only_window_id),
        uint32_t(RemoteProgramsWindowIdManager::INVALID_WINDOW_ID));


    uint32_t server_window_id = client_only_window_id;
    uint32_t client_window_id = rail_window_id_manager.get_client_window_id_ex(server_window_id);

    BOOST_CHECK(server_window_id != client_window_id);

    BOOST_CHECK_EQUAL(rail_window_id_manager.get_client_window_id(server_window_id), client_window_id);
    BOOST_CHECK_EQUAL(rail_window_id_manager.get_server_window_id(client_window_id), server_window_id);


    rail_window_id_manager.unregister_server_window(server_window_id);
}

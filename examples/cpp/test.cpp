#include <rxcpp/rx.hpp>
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include "kvo_extension.hpp"

#pragma warning(disable:4503)


SCENARIO("test kvo_varibale", "")
{
    struct Person
    {
        kvo::variable<std::string> firstName;
        kvo::variable<std::string> lastName;
        kvo::variable<std::string> fullName;
        
        Person()
        {
            firstName.subject.get_observable()
            .combine_latest(lastName.subject.get_observable())
            .subscribe([this](std::tuple<std::string,std::string>x){
                this->fullName = std::get<0>(x) + " " + std::get<1>(x);
            });
        }
        Person(std::string _firstName, std::string _lastName):Person()
        {
            this->firstName = _firstName;
            this->lastName = _lastName;
        }
    };
    GIVEN("a person")
    {
        Person person;
        REQUIRE(person.firstName.get() == "");
        REQUIRE(person.lastName.get() == "");
        REQUIRE(person.fullName.get() == " ");
        WHEN("modify firstName")
        {
            person.firstName = "Hello";
            REQUIRE(person.firstName.get() == "Hello");
            REQUIRE(person.lastName.get() == "");
            REQUIRE(person.fullName.get() == "Hello ");
            THEN("modify lastName")
            {
                person.lastName = "World";
                REQUIRE(person.firstName.get() == "Hello");
                REQUIRE(person.lastName.get() == "World");
                REQUIRE(person.fullName.get() == "Hello World");
            }
        }
        WHEN("modify lastName")
        {
            person.lastName = "World";
            REQUIRE(person.firstName.get() == "");
            REQUIRE(person.lastName.get() == "World");
            REQUIRE(person.fullName.get() == " World");
            THEN("modify firstName")
            {
                person.firstName = "Hello";
                REQUIRE(person.firstName.get() == "Hello");
                REQUIRE(person.lastName.get() == "World");
                REQUIRE(person.fullName.get() == "Hello World");
            }
        }
    }
    GIVEN("a leader")
    {
        struct Leader
        {
			Leader() {}
            kvo::variable<Person> member;
        };
        AND_THEN("given a team with rx key path")
        {
            struct Team
            {
                kvo::variable<Leader> leader;
                kvo::variable<std::string> leader_fullName;
                Team()
                {
                    leader.subject.get_observable()
                    .map([](const Leader&x){ return x.member.subject.get_observable(); }).switch_on_next()
                    .map([](const Person&x){ return x.fullName.subject.get_observable(); }).switch_on_next()
                    .subscribe([this](const std::string&x){ this->leader_fullName = x; });
                }
            }team;
            
            REQUIRE(team.leader_fullName() == " ");
            
            REQUIRE(team.leader().member().firstName() == "");
            REQUIRE(team.leader().member().lastName() == "");
            REQUIRE(team.leader().member().fullName() == " ");
            
            WHEN("modify leader's member")
            {
                team.leader().member = Person("Hello", "World");
                REQUIRE(team.leader().member().firstName() == "Hello");
                REQUIRE(team.leader().member().lastName() == "World");
                REQUIRE(team.leader().member().fullName() == "Hello World");
                REQUIRE(team.leader_fullName() == "Hello World");
                
                AND_WHEN("modify leader's member's firstName")
                {
                    team.leader().member().firstName = "Good";
                    REQUIRE(team.leader().member().firstName() == "Good");
                    REQUIRE(team.leader().member().lastName() == "World");
                    REQUIRE(team.leader().member().fullName() == "Good World");
                    REQUIRE(team.leader_fullName() == "Good World");
                }
                AND_WHEN("modify leader's member's lastName")
                {
                    team.leader().member().lastName = "Luck";
                    REQUIRE(team.leader().member().firstName() == "Hello");
                    REQUIRE(team.leader().member().lastName() == "Luck");
                    REQUIRE(team.leader().member().fullName() == "Hello Luck");
                    REQUIRE(team.leader_fullName() == "Hello Luck");
                }
            }
        }
        AND_THEN("given a team with simplify key path")
        {
            struct Team
            {
                kvo::variable<Leader> leader;
                kvo::variable<std::string> leader_fullName;
                Team()
                {
                    leader([](const Leader&x){ return x.member.subject.get_observable(); },
                           [](const Person&x){ return x.fullName.subject.get_observable(); })
                    .subscribe([this](const std::string&x){
                        this->leader_fullName = x;
                    }, [](std::exception_ptr){});
                }
            }team;
            
            REQUIRE(team.leader_fullName() == " ");
            
            REQUIRE(team.leader().member().firstName() == "");
            REQUIRE(team.leader().member().lastName() == "");
            REQUIRE(team.leader().member().fullName() == " ");
            
            WHEN("modify leader's member")
            {
                team.leader().member = Person("Hello", "World");
                REQUIRE(team.leader().member().firstName() == "Hello");
                REQUIRE(team.leader().member().lastName() == "World");
                REQUIRE(team.leader().member().fullName() == "Hello World");
                REQUIRE(team.leader_fullName() == "Hello World");
                
                AND_WHEN("modify leader's member's firstName")
                {
                    team.leader().member().firstName = "Good";
                    REQUIRE(team.leader().member().firstName() == "Good");
                    REQUIRE(team.leader().member().lastName() == "World");
                    REQUIRE(team.leader().member().fullName() == "Good World");
                    REQUIRE(team.leader_fullName() == "Good World");
                }
                AND_WHEN("modify leader's member's lastName")
                {
                    team.leader().member().lastName = "Luck";
                    REQUIRE(team.leader().member().firstName() == "Hello");
                    REQUIRE(team.leader().member().lastName() == "Luck");
                    REQUIRE(team.leader().member().fullName() == "Hello Luck");
                    REQUIRE(team.leader_fullName() == "Hello Luck");
                }
            }
        }
    }
}

SCENARIO("test basic kvo_collection operations", "")
{
    GIVEN("a kvo_collection with std::vector")
    {
        typedef std::vector<int> collection_t;
        kvo::collection<collection_t> IDs;
        std::size_t ID_count = 0;
        THEN("watch IDs' count")
        {
            typedef decltype(IDs)::rx_notify_value rx_notify_value;
            rxcpp::observable<>::empty<rx_notify_value>()
            .merge(IDs.subject_setting.get_observable(),
                   IDs.subject_insertion.get_observable(),
                   IDs.subject_removal.get_observable(),
                   IDs.subject_replacement.get_observable())
            .subscribe([&ID_count, &IDs](const rx_notify_value&x){
                ID_count = IDs().size();
            });
            REQUIRE(ID_count == 0);
            
            WHEN("set a list of data")
            {
                IDs.set({100,200,300,400,500,600,700,800,900});
                REQUIRE(IDs.get() == collection_t({100,200,300,400,500,600,700,800,900}));
                REQUIRE(IDs() == collection_t({100,200,300,400,500,600,700,800,900}));
                REQUIRE(ID_count == 9);
                
                IDs.set({100,200,300,400});
                REQUIRE(IDs.get() == collection_t({100,200,300,400}));
                REQUIRE(IDs() == collection_t({100,200,300,400}));
                REQUIRE(ID_count == 4);
                
                THEN("insert {99,88} at {1, 3}")
                {
                    IDs.insert({99, 88}, {1, 3});
                    REQUIRE(IDs() == collection_t({100,99,200,88,300,400}));
                    REQUIRE(ID_count == 6);
                }
                THEN("insert {99,88} at {4, 5}")
                {
                    IDs.insert({99, 88}, {4, 5});
                    REQUIRE(IDs() == collection_t({100,200,300,400,99,88}));
                    REQUIRE(ID_count == 6);
                }
                THEN("insert {99,88,77} at {1, 2, 4}")
                {
                    IDs.insert({99, 88, 77}, {1, 2, 4});
                    REQUIRE(IDs() == collection_t({100,99,88,200,77,300,400}));
                    REQUIRE(ID_count == 7);
                }
                
                THEN("insert {99,88,77} at end")
                {
                    IDs.insert({99, 88, 77});
                    REQUIRE(IDs() == collection_t({100,200,300,400,99,88,77}));
                    REQUIRE(ID_count == 7);
                }
                
                THEN("remove objects at indices {1,3}")
                {
                    IDs.remove({1, 3});
                    REQUIRE(IDs() == collection_t({100,300}));
                    REQUIRE(ID_count == 2);
                }
                
                THEN("replace objects at indices {1,3}")
                {
                    IDs.replace({1, 3}, {99, 88});
                    REQUIRE(IDs() == collection_t({100,99,300,88}));
                    REQUIRE(ID_count == 4);
                }
                
                THEN("remove all items")
                {
                    IDs.set({});
                    REQUIRE(IDs() == collection_t({}));
                    REQUIRE(ID_count == 0);
                }
                
                THEN("remove all items")
                {
                    IDs.remove_all();
                    REQUIRE(IDs() == collection_t({}));
                    REQUIRE(ID_count == 0);
                }
            }
        }
    }
    GIVEN("a kvo_collection with std::list")
    {
        typedef std::list<int> collection_t;
        kvo::collection<collection_t> IDs;
        std::size_t ID_count = 0;
        THEN("watch IDs' count")
        {
            typedef decltype(IDs)::rx_notify_value rx_notify_value;
            rxcpp::observable<>::empty<rx_notify_value>()
            .merge(IDs.subject_setting.get_observable(),
                   IDs.subject_insertion.get_observable(),
                   IDs.subject_removal.get_observable(),
                   IDs.subject_replacement.get_observable())
            .subscribe([&ID_count, &IDs](const rx_notify_value&x){
                ID_count = IDs().size();
            });
            REQUIRE(ID_count == 0);
            
            WHEN("set a list of data")
            {
                IDs.set({100,200,300,400,500,600,700,800,900});
                REQUIRE(IDs.get() == collection_t({100,200,300,400,500,600,700,800,900}));
                REQUIRE(IDs() == collection_t({100,200,300,400,500,600,700,800,900}));
                REQUIRE(ID_count == 9);
                
                IDs.set({100,200,300,400});
                REQUIRE(IDs.get() == collection_t({100,200,300,400}));
                REQUIRE(IDs() == collection_t({100,200,300,400}));
                REQUIRE(ID_count == 4);
                
                THEN("insert {99,88} at {1, 3}")
                {
                    IDs.insert({99, 88}, {1, 3});
                    REQUIRE(IDs() == collection_t({100,99,200,88,300,400}));
                    REQUIRE(ID_count == 6);
                }
                THEN("insert {99,88} at {4, 5}")
                {
                    IDs.insert({99, 88}, {4, 5});
                    REQUIRE(IDs() == collection_t({100,200,300,400,99,88}));
                    REQUIRE(ID_count == 6);
                }
                THEN("insert {99,88,77} at {1, 2, 4}")
                {
                    IDs.insert({99, 88, 77}, {1, 2, 4});
                    REQUIRE(IDs() == collection_t({100,99,88,200,77,300,400}));
                    REQUIRE(ID_count == 7);
                }
                
                THEN("insert {99,88,77} at end")
                {
                    IDs.insert({99, 88, 77});
                    REQUIRE(IDs() == collection_t({100,200,300,400,99,88,77}));
                    REQUIRE(ID_count == 7);
                }
                
                THEN("remove objects at indices {1,3}")
                {
                    IDs.remove({1, 3});
                    REQUIRE(IDs() == collection_t({100,300}));
                    REQUIRE(ID_count == 2);
                }
                
                THEN("replace objects at indices {1,3}")
                {
                    IDs.replace({1, 3}, {99, 88});
                    REQUIRE(IDs() == collection_t({100,99,300,88}));
                    REQUIRE(ID_count == 4);
                }
                
                THEN("remove all items")
                {
                    IDs.set({});
                    REQUIRE(IDs() == collection_t({}));
                    REQUIRE(ID_count == 0);
                }
                
                THEN("remove all items")
                {
                    IDs.remove_all();
                    REQUIRE(IDs() == collection_t({}));
                    REQUIRE(ID_count == 0);
                }
            }
        }
    }
    GIVEN("a kvo_collection with std::set")
    {
        typedef std::set<int> collection_t;
        kvo::collection<collection_t> IDs;
		std::size_t ID_count = 0;
        THEN("watch IDs' count")
        {
            typedef decltype(IDs)::rx_notify_value rx_notify_value;
            rxcpp::observable<>::empty<rx_notify_value>()
            .merge(IDs.subject_setting.get_observable(),
                   IDs.subject_insertion.get_observable(),
                   IDs.subject_removal.get_observable(),
                   IDs.subject_replacement.get_observable())
            .subscribe([&ID_count, &IDs](const rx_notify_value&x){
                ID_count = IDs().size();
            });
            REQUIRE(ID_count == 0);
            
            WHEN("set a set of data")
            {
                IDs.set({100,200,300,400,500,600,700,800,900});
                REQUIRE(IDs.get() == collection_t({100,200,300,400,500,600,700,800,900}));
                REQUIRE(IDs() == collection_t({100,200,300,400,500,600,700,800,900}));
                REQUIRE(ID_count == 9);
                
                IDs.set({100,200,300,400});
                REQUIRE(IDs.get() == collection_t({100,200,300,400}));
                REQUIRE(IDs() == collection_t({100,200,300,400}));
                REQUIRE(ID_count == 4);
                
                THEN("insert {99,88}")
                {
                    IDs.insert({99, 88});
                    REQUIRE(IDs() == collection_t({100,99,200,88,300,400}));
                    REQUIRE(ID_count == 6);
                }
                
                THEN("remove objects {99, 88, 300}")
                {
                    IDs.remove({99, 88, 300});
                    REQUIRE(IDs() == collection_t({100,200,400}));
                    REQUIRE(ID_count == 3);
                }
                
                THEN("replace objects {200, 400} with {99, 88}")
                {
                    IDs.replace({200, 400}, {99, 88});
                    REQUIRE(IDs() == collection_t({100,99,300,88}));
                    REQUIRE(ID_count == 4);
                }
                
                THEN("remove all items")
                {
                    IDs.set({});
                    REQUIRE(IDs() == collection_t({}));
                    REQUIRE(ID_count == 0);
                }
                
                THEN("remove all items")
                {
                    IDs.remove_all();
                    REQUIRE(IDs() == collection_t({}));
                    REQUIRE(ID_count == 0);
                }
            }
        }
    }
    GIVEN("a kvo_collection with std::unordered_set")
    {
        typedef std::unordered_set<int> collection_t;
        kvo::collection<collection_t> IDs;
		std::size_t ID_count = 0;
        THEN("watch IDs' count")
        {
            typedef decltype(IDs)::rx_notify_value rx_notify_value;
            rxcpp::observable<>::empty<rx_notify_value>()
            .merge(IDs.subject_setting.get_observable(),
                   IDs.subject_insertion.get_observable(),
                   IDs.subject_removal.get_observable(),
                   IDs.subject_replacement.get_observable())
            .subscribe([&ID_count, &IDs](const rx_notify_value&x){
                ID_count = IDs().size();
            });
            REQUIRE(ID_count == 0);
            
            WHEN("set a set of data")
            {
                IDs.set({100,200,300,400,500,600,700,800,900});
                REQUIRE(IDs.get() == collection_t({100,200,300,400,500,600,700,800,900}));
                REQUIRE(IDs() == collection_t({100,200,300,400,500,600,700,800,900}));
                REQUIRE(ID_count == 9);
                
                IDs.set({100,200,300,400});
                REQUIRE(IDs.get() == collection_t({100,200,300,400}));
                REQUIRE(IDs() == collection_t({100,200,300,400}));
                REQUIRE(ID_count == 4);
                
                THEN("insert {99,88}")
                {
                    IDs.insert({99, 88});
                    REQUIRE(IDs() == collection_t({100,99,200,88,300,400}));
                    REQUIRE(ID_count == 6);
                }
                
                THEN("remove objects {99, 88, 300}")
                {
                    IDs.remove({99, 88, 300});
                    REQUIRE(IDs() == collection_t({100,200,400}));
                    REQUIRE(ID_count == 3);
                }
                
                THEN("replace objects {200, 400} with {99, 88}")
                {
                    IDs.replace({200, 400}, {99, 88});
                    REQUIRE(IDs() == collection_t({100,99,300,88}));
                    REQUIRE(ID_count == 4);
                }
                
                THEN("remove all items")
                {
                    IDs.set({});
                    REQUIRE(IDs() == collection_t({}));
                    REQUIRE(ID_count == 0);
                }
                
                THEN("remove all items")
                {
                    IDs.remove_all();
                    REQUIRE(IDs() == collection_t({}));
                    REQUIRE(ID_count == 0);
                }
            }
        }
    }
}

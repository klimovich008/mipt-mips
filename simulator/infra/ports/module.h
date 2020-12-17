/**
 * module.h - module template
 * @author Pavel Kryukov
 * Copyright 2019 MIPT-V team
 */

#ifndef INFRA_PORTS_MODULE_H
#define INFRA_PORTS_MODULE_H
 
#include <infra/log.h>
#include <infra/ports/ports.h>

#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional/optional.hpp>
#include <map>

#include <set>
#include <unordered_set>
#include <modules/core/perf_instr.h>

class Module : public Log
{
public:
    Module( Module* parent, std::string name);

    void static init_track_data(uint64 first_cycle, uint64 last_cycle);
    void static save_track_to_file(std::string filename);

protected:

    template <typename FuncInstr>
    void static init_record(const PerfInstr<FuncInstr> &instr, const Cycle &cycle)
    {
        if (!json_track_data.length())
            return;

        switch ((track_first_cycle ? 1 : 0) + (track_last_cycle ? 2 : 0))
        {
        case 0:
            tracked_instr.insert(std::make_pair(instr.get_sequence_id(), record_id++));
            break;
        case 1:
            if (double(cycle) >= track_first_cycle)
                tracked_instr.insert(std::make_pair(instr.get_sequence_id(), record_id++));
            else
                return;
            break;
        case 2:
            if (double(cycle) <= track_last_cycle)
                tracked_instr.insert(std::make_pair(instr.get_sequence_id(), record_id++));
            else
                return;
            break;
        case 3:
            if (double(cycle) <= track_last_cycle && double(cycle) >= track_first_cycle)
                tracked_instr.insert(std::make_pair(instr.get_sequence_id(), record_id++));
            else
                return;
            break;
        }

        json_track_data += "\t{ \"type\": \"Record\", \"id\": " + std::to_string(record_id - 1) + ", \"disassembly\": \"" + instr.get_disasm() + "\" },\n";
    }

    template <typename FuncInstr>
    void event(const PerfInstr<FuncInstr> &instr, const Cycle &cycle, int stage_id)
    {
        if (!json_track_data.length())
            return;
        if (!tracked_instr.contains(instr.get_sequence_id()))
            return;

        json_track_data += "\t{ \"type\": \"Event\", \"id\": " + std::to_string(tracked_instr.find(instr.get_sequence_id())->second) + ", \"cycle\": " + cycle.to_string() + ", \"stage\": " + std::to_string(stage_id) + " },\n";

        if(stage_id == 4) tracked_instr.erase(instr.get_sequence_id());
    }

    template<typename T>
    auto make_write_port( std::string key, uint32 bandwidth) 
    {
        auto port = std::make_unique<WritePort<T>>( get_portmap(), std::move(key), bandwidth);
        auto ptr = port.get();
        write_ports.emplace_back( std::move( port));
        return ptr;
    }

    template<typename T>
    auto make_read_port( std::string key, Latency latency)
    {
        auto port = std::make_unique<ReadPort<T>>( get_portmap(), std::move(key), latency);
        auto ptr = port.get();
        read_ports.emplace_back( std::move( port));
        return ptr;
    }

    void enable_logging_impl( const std::unordered_set<std::string>& names);
    boost::property_tree::ptree topology_dumping_impl() const;

private:
    // NOLINTNEXTLINE(misc-no-recursion) Recursive, but must be finite
    virtual std::shared_ptr<PortMap> get_portmap() const { return parent->get_portmap(); }
    void force_enable_logging();
    void force_disable_logging();

    void add_child( Module* module) { children.push_back( module); }

    virtual boost::property_tree::ptree portmap_dumping() const;
    boost::property_tree::ptree read_ports_dumping() const;
    boost::property_tree::ptree write_ports_dumping() const;

    void module_dumping( boost::property_tree::ptree* modules) const;
    void modulemap_dumping( boost::property_tree::ptree* modulemap) const;

    Module* const parent;
    std::vector<Module*> children;
    const std::string name;
    std::vector<std::unique_ptr<BasicWritePort>> write_ports;
    std::vector<std::unique_ptr<BasicReadPort>> read_ports;
    static uint64 track_first_cycle;
    static uint64 track_last_cycle;
    static std::string json_track_data;
    static int record_id;
    static std::map<int, int> tracked_instr;
};

class Root : public Module
{
public:
    explicit Root( std::string name)
        : Module( nullptr, std::move( name))
        , portmap( PortMap::create_port_map())
    { }

protected:
    void init_portmap() { portmap->init(); }
    void enable_logging( const std::string& values);
    
    void topology_dumping( bool dump, const std::string& filename);

private:
    std::shared_ptr<PortMap> get_portmap() const final { return portmap; }
    std::shared_ptr<PortMap> portmap;
    boost::property_tree::ptree portmap_dumping() const final;
};

#endif

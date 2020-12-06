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

    void static save_track_to_file(std::string filename)
    {
        if(!filename.length()) return; 

        boost::property_tree::ptree array;
        for (auto record : json_track_data)
        {
            array.push_back(std::make_pair("", record.second));
        }
        boost::property_tree::ptree data;
        data.add_child("data", array);
        boost::property_tree::write_json(filename + ".json", data);
    }

protected:
    template <typename FuncInstr>
    void init_record(const PerfInstr<FuncInstr> &instr)
    {
        boost::property_tree::ptree new_record;

        new_record.put("type", "Record");
        new_record.put("id", instr.get_sequence_id());
        new_record.add("disassembly", instr.get_disasm());

        json_track_data.insert(std::make_pair(instr.get_sequence_id(), new_record));
    }

    template <typename FuncInstr>
    void add_stage_to_record(const PerfInstr<FuncInstr> &instr, std::string description, Cycle cycle)
    {
        boost::property_tree::ptree &record = json_track_data[instr.get_sequence_id()];
        boost::property_tree::ptree data;
        data.put("cycle", cycle);
        data.put("description", description);

        boost::property_tree::ptree data_array;

        data_array.push_back(boost::property_tree::ptree::value_type("", data));

        boost::optional<boost::property_tree::ptree &> array =
            record.get_child_optional("stages");
        if (array)
            array->push_back(boost::property_tree::ptree::value_type("", data));
        else
            record.put_child("stages", data_array);
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
    static std::map<int, boost::property_tree::ptree> json_track_data;
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

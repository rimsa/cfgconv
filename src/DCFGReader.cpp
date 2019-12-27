/*

   Copyright (C) 2019, Andrei Rimsa (andrei@cefetmg.br)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
*/

#include <list>
#include <string>
#include <CFG.h>
#include <CfgNode.h>
#include <DCFGReader.h>

using json = nlohmann::json;

DCFGReader::DCFGReader(const std::string& filename)
	: CFGReader(filename) {
}

DCFGReader::~DCFGReader() {
}

static
std::vector<std::string> readStrings(json& obj, const std::string& field) {
	json::iterator it, ed;
	std::vector<std::string> container;

	assert(obj.is_object());

	ed = obj.end();
	it = obj.find(field);
	assert(it != ed);

	json& array = it.value();
	assert(array.is_array());

	for (it = ++(array.begin()), ed = array.end(); it != ed; ++it) {
		int id = (*it).at(0);
		const std::string& name = (*it).at(1);
		if (container.size() <= id)
			container.resize(id+1);

		container[id] = name;
	}

	return container;
}

void DCFGReader::buildCFG(int entry) {
	CFG* cfg = this->instance(m_nodes[entry].addr);

	// Add special entry node.
	assert(cfg->entryNode() == 0);
	CfgNode* entry_node = new CfgNode(CfgNode::CFG_ENTRY);
	cfg->addNode(entry_node);

	// Add the first node.
	assert(cfg->nodeByAddr(m_nodes[entry].addr) == 0);
	cfg->addEdge(entry_node, CFGReader::nodeWithAddr(cfg, m_nodes[entry].addr),
					cfg->execs());

	std::set<int> visited;
	std::list<int> worklist;
	worklist.push_back(entry);

	while (!worklist.empty()) {
		int src_id = worklist.front();
		worklist.pop_front();
		assert(src_id > 3);

		if (visited.find(src_id) != visited.cend())
			continue;
		visited.insert(src_id);

		DCFGReader::Node& src_bb = m_nodes[src_id];
		CfgNode* src_node = cfg->nodeByAddr(src_bb.addr);
		assert(src_node != 0 && src_node->type() == CfgNode::CFG_PHANTOM);
		src_node->setData(new CfgNode::BlockData(src_bb.addr, src_bb.size));

		for (DCFGReader::Edge& edge : m_edges[src_id]) {
			// Ignore unknown node.
			if (edge.dst_id == UNKNOWN_NODE)
				continue;

			std::map<int, Node>::const_iterator it = m_nodes.find(edge.dst_id);
			Addr dst_addr = (it != m_nodes.end() ? (it->second).addr : 0);

			unsigned long long count = edge.count;

			switch (edge.edge_type) {
				case INDIRECT_UNCONDITIONAL_BRANCH_EDGE:
					CFGReader::markIndirect(src_node);
					// fallthrough.
				case REP_EDGE:
				case CALL_BYPASS_EDGE:
				case SYSTEM_CALL_BYPASS_EDGE:
				case DIRECT_UNCONDITIONAL_BRANCH_EDGE:
				case FALL_THROUGH_EDGE:
				case EXCLUDED_CODE_BYPASS_EDGE:
					cfg->addEdge(src_node, CFGReader::nodeWithAddr(cfg, dst_addr), count);
					worklist.push_back(edge.dst_id);
					break;
				case DIRECT_CONDITIONAL_BRANCH_EDGE:
					cfg->addEdge(src_node, CFGReader::nodeWithAddr(cfg, dst_addr), count);
					cfg->addEdge(src_node, CFGReader::nodeWithAddr(cfg, src_bb.addr + src_bb.size));
					worklist.push_back(edge.dst_id);
					break;
				case INDIRECT_CALL_EDGE:
					CFGReader::markIndirect(src_node);
					// fallthrough
				case SYSTEM_CALL_EDGE:
				case DIRECT_CALL_EDGE: {
					CFG* called = this->instance(dst_addr);
					CFGReader::addCall(src_node, called, count,
						m_visited.find(src_id) == m_visited.cend());
					} break;
				case EXIT_EDGE:
					// The destination must be the special exit node (2).
					assert(edge.dst_id == 2);
					cfg->addEdge(src_node, CFGReader::haltNode(cfg), count);
					break;
				case RETURN_EDGE:
					cfg->addEdge(src_node, CFGReader::exitNode(cfg), count);
					break;
				case CONTEXT_CHANGE_EDGE: {
					CFG* sigHandler = this->instance(dst_addr);
					CFGReader::addSignalHandler(src_node, 0, sigHandler, count,
						m_visited.find(src_id) == m_visited.cend());

					} break;
				default: {
					// std::vector<std::string> edgeTypes = readStrings(obj, "EDGE_TYPES");
					// std::cout << "edge type: " << edgeTypes[edge.edge_type] << "\n";
					assert(false);
					} break;
			}
		}

		// Added to the global visited ids.
		m_visited.insert(src_id);
	}
}

void DCFGReader::loadCFGs() {
	json obj;

	// We consider the first block (4, the first id after the special ones)
	// as an CFG entry.
	m_entries.insert(4);

	m_input >> obj;

	m_filenames = readStrings(obj, "FILE_NAMES");
	readProcesses(obj);

	for (int entry : m_entries)
		this->buildCFG(entry);

	for (CFG* cfg : this->cfgs())
		cfg->check();
}

static
Addr str2addr(const std::string& str) {
	assert(str.compare(0, 2, "0x") == 0);
	Addr addr = std::stoul(str.substr(2), 0, 16);
	assert(addr != 0);

	return addr;
}

void DCFGReader::readProcesses(json& obj) {
	json::iterator it, ed;

	assert(obj.is_object());

	ed = obj.end();
	it = obj.find("PROCESSES");
	assert(it != ed);

	json& array = it.value();
	assert(array.is_array());

	for (it = ++(array.begin()), ed = array.end(); it != ed; ++it) {
		json& pdata = (*it).at(1);
		assert(pdata.is_object());

		json::iterator it2;
		it2 = pdata.find("IMAGES");
		assert(it2 != pdata.end());
		readImages(it2.value());

		it2 = pdata.find("EDGES");
		assert(it2 != pdata.end());
		readEdges(it2.value());
	}
}

void DCFGReader::readImages(json& array) {
	json::iterator it, ed;

	assert(array.is_array());

	for (it = ++(array.begin()), ed = array.end(); it != ed; ++it) {
		Addr baseAddr = str2addr((*it).at(1));

		json& idata = (*it).at(3);
		assert(idata.is_object());

		json::iterator it2 = idata.find("FILE_NAME_ID");
		assert(it2 != idata.end());
		int file_id = it2.value();

		it2 = idata.find("BASIC_BLOCKS");
		if (it2 != idata.end())
			readBasicBlocks(baseAddr, it2.value());

		it2 = idata.find("ROUTINES");
		if (it2 != idata.end())
			readRoutines(it2.value());

		it2 = idata.find("SYMBOLS");
		if (it2 != idata.end())
			readSymbols(file_id, baseAddr, it2.value());

		it2 = idata.find("SOURCE_DATA");
		if (it2 != idata.end())
			readSourceData(file_id, baseAddr, it2.value());
	}
}

void DCFGReader::readBasicBlocks(Addr baseAddr, json& array) {
	json::iterator it, ed;

	assert(array.is_array());

	for (it = ++(array.begin()), ed = array.end(); it != ed; ++it) {
		Addr addr = str2addr((*it).at(1));
		int id = (*it).at(0);
		m_nodes[id] = (DCFGReader::Node) {
			.addr = baseAddr + addr,
			.size = (*it).at(2),
			.instrs = (*it).at(3),
			.execs = (*it).at(5)
		};
	}
}

void DCFGReader::readRoutines(json& array) {
	json::iterator it, ed;

	assert(array.is_array());

	for (it = ++(array.begin()), ed = array.end(); it != ed; ++it) {
		json::iterator it2, ed2;
		DCFGReader::Routine r;

		r.entry_bb = (*it).at(0);

		json& tmp = (*it).at(1);
		assert(tmp.is_array());
		for (json& element : tmp)
			r.exit_bbs.insert(static_cast<int>(element));

		tmp = (*it).at(2);
		assert(tmp.is_array());

		for (it2 = ++(tmp.begin()), ed2 = tmp.end(); it2 != ed2; ++it2)
			r.bbs.insert(static_cast<int>((*it2).at(0)));

		m_routines.push_back(r);
	}
}

void DCFGReader::readSymbols(int file_id, Addr baseAddr, json& array) {
	json::iterator it, ed;

	assert(array.is_array());

	for (it = ++(array.begin()), ed = array.end(); it != ed; ++it) {
		Addr addr = str2addr((*it).at(1));
		m_symbols[baseAddr + addr] = (DCFGReader::Symbol) {
			.file_id = file_id,
			.fname = (*it).at(0),
			.lineno = -1
		};
	}
}

void DCFGReader::readSourceData(int file_id, Addr baseAddr, json& array) {
	json::iterator it, ed;

	assert(array.is_array());

	for (it = ++(array.begin()), ed = array.end(); it != ed; ++it) {
		Addr addr = str2addr((*it).at(2));
		std::map<Addr, Symbol>::iterator it2 = m_symbols.find(baseAddr + addr);
		if (it2 != m_symbols.end()) {
			DCFGReader::Symbol& s = it2->second;
			s.file_id = (*it).at(0);
			s.lineno = (*it).at(1);
		}
	}
}

void DCFGReader::readEdges(json& array) {
	json::iterator it, ed;

	assert(array.is_array());

	for (it = ++(array.begin()), ed = array.end(); it != ed; ++it) {
		int src = (*it).at(1);
		int dst = (*it).at(2);
		int etype = (*it).at(3);
		json& counts = (*it).at(4);

		json::iterator it2, ed2;
		unsigned long long count = 0;
		for (it2 = counts.begin(), ed2 = counts.end(); it2 != ed2; ++it2)
			count += static_cast<int>(*it2);

		m_edges[src].push_back((DCFGReader::Edge) {
			.dst_id = dst,
			.edge_type = etype,
			.count = count
		});

		if (dst > 3) {
			// Targets of call and context changes are CFG entries.
			switch (etype) {
				case INDIRECT_CALL_EDGE:
				case SYSTEM_CALL_EDGE:
				case DIRECT_CALL_EDGE:
				case CONTEXT_CHANGE_EDGE:
					m_entries.insert(dst);
					break;
				default:
					break;
			}
		}
	}
}

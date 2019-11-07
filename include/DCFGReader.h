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

#ifndef DCFG_READER_H
#define DCFG_READER_H

#include <set>
#include <list>
#include <vector>
#include <CFGReader.h>
#include <nlohmann/json.hpp>

class DCFGReader : public CFGReader {
public:
	DCFGReader(const std::string& filename);
	virtual ~DCFGReader();

	virtual void loadCFGs();

private:
	enum NodeType {
		ENTRY_NODE = 1,
		EXIT_NODE,
		UNKNOWN_NODE
	};

	enum EdgeType {
		ENTRY_EDGE = 1,
		EXIT_EDGE,
		CALL_EDGE,
		DIRECT_CALL_EDGE,
		INDIRECT_CALL_EDGE,
		RETURN_EDGE,
		CALL_BYPASS_EDGE,
		BRANCH_EDGE,
		CONDITIONAL_BRANCH_EDGE,
		UNCONDITIONAL_BRANCH_EDGE,
		DIRECT_BRANCH_EDGE,
		INDIRECT_BRANCH_EDGE,
		DIRECT_CONDITIONAL_BRANCH_EDGE,
		INDIRECT_CONDITIONAL_BRANCH_EDGE,
		DIRECT_UNCONDITIONAL_BRANCH_EDGE,
		INDIRECT_UNCONDITIONAL_BRANCH_EDGE,
		REP_EDGE,
		FALL_THROUGH_EDGE,
		SYSTEM_CALL_EDGE,
		SYSTEM_RETURN_EDGE,
		SYSTEM_CALL_BYPASS_EDGE,
		CONTEXT_CHANGE_EDGE,
		CONTEXT_CHANGE_RETURN_EDGE,
		CONTEXT_CHANGE_BYPASS_EDGE,
		EXCLUDED_CODE_BYPASS_EDGE,
		UNKNOWN_EDGE
	};

	struct Node {
		Addr addr;
		int size;
		int instrs;
		int execs;
	};

	struct Edge {
		int dst_id;
		int edge_type;
	};

	struct Symbol {
		int file_id;
		std::string fname;
		int lineno;
	};

	struct Routine {
		std::set<int> bbs;
		int entry_bb;
		std::set<int> exit_bbs;
	};

	std::vector<std::string> m_filenames;
	std::map<int, Node> m_nodes;
	std::map<int, std::list<Edge>> m_edges;
	std::list<Routine> m_routines;
	std::map<Addr, Symbol> m_symbols;

	void readProcesses(nlohmann::json& obj);
	void readImages(nlohmann::json& array);
	void readBasicBlocks(Addr baseAddr, nlohmann::json& array);
	void readRoutines(nlohmann::json& array);
	void readSymbols(int file_id, Addr baseAddr, nlohmann::json& array);
	void readSourceData(int file_id, Addr baseAddr, nlohmann::json& array);
	void readEdges(nlohmann::json& array);

};

#endif
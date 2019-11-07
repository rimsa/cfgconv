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

#ifndef CFG_H
#define CFG_H

#include <set>
#include <map>
#include <string>
#include <fstream>

#include <Addr.h>

class CfgNode;
class CfgEdge;

class CFG {
public:
	enum Status {
		UNCHECKED,
		INVALID,
		VALID
	};

	CFG(Addr addr);
	virtual ~CFG();

	Addr addr() const { return m_addr; }

	const std::string& functionName() const { return m_functionName; }
	void setFunctionName(const std::string& functionName);

	bool complete() const;

	CfgNode* entryNode() const { return m_entryNode; }
	CfgNode* exitNode() const { return m_exitNode; }
	CfgNode* haltNode() const { return m_haltNode; }
	CfgNode* nodeByAddr(Addr addr) const;

	const std::set<CfgNode*>& nodes() const { return m_nodes; }
	bool containsNode(CfgNode* node) const;
	void addNode(CfgNode* node);

	const std::set<CfgEdge*>& edges() const { return m_edges; }
	bool containsEdge(CfgNode* src, CfgNode* dst) const;
	void addEdge(CfgNode* src, CfgNode* dst);

	const std::set<CfgNode*>& successors(CfgNode* node) const;
	const std::set<CfgNode*>& predecessors(CfgNode* node) const;

	enum Status status() const { return m_status; }
	enum CFG::Status check();

	std::string toDOT() const;
	void dumpDOT(const std::string& fileName);

	std::string str() const;
	friend std::ostream& operator<<(std::ostream& os, const CFG& cfg);

private:
	Addr m_addr;
	enum Status m_status;
	std::string m_functionName;
	bool m_complete;

	CfgNode* m_entryNode;
	CfgNode* m_exitNode;
	CfgNode* m_haltNode;
	std::set<CfgNode*> m_nodes;
	std::map<Addr, CfgNode*> m_nodesMap;

	std::set<CfgEdge*> m_edges;
	std::map<CfgNode*, std::set<CfgNode*>> m_succs;
	std::map<CfgNode*, std::set<CfgNode*>> m_preds;

};

#endif

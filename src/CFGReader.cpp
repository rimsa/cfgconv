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

#include <cassert>
#include <CFG.h>
#include <CfgEdge.h>
#include <CfgNode.h>
#include <CFGReader.h>

CFGReader::CFGReader(const std::string& filename)
	: m_input(filename, std::fstream::in) {
}

CFGReader::~CFGReader() {
	m_input.close();

	for (std::map<Addr, CFG*>::iterator it = m_cfgs.begin(),
			ed = m_cfgs.end(); it != ed; it++) {
		delete it->second;
	}
}

std::set<CFG*> CFGReader::cfgs() const {
	std::set<CFG*> cfgs;

	std::transform(m_cfgs.begin(), m_cfgs.end(),
		std::inserter(cfgs, cfgs.begin()),
		[](const std::map<Addr, CFG*>::value_type &pair) {
			return pair.second;
		}
	);

	return cfgs;
}

CFG* CFGReader::cfg(Addr addr) const {
	std::map<Addr, CFG*>::const_iterator it = m_cfgs.find(addr);
	return it != m_cfgs.end() ? it->second : 0;
}

CFG* CFGReader::instance(Addr addr) {
	CFG* cfg = this->cfg(addr);
	if (cfg == 0) {
		cfg = new CFG(addr);
		m_cfgs[addr] = cfg;
	}

	return cfg;
}

CfgNode* CFGReader::entryNode(CFG* cfg) {
	CfgNode* entry_node = cfg->entryNode();
	if (entry_node == 0) {
		entry_node = new CfgNode(CfgNode::CFG_ENTRY);
		cfg->addNode(entry_node);
	}

	return entry_node;
}

CfgNode* CFGReader::nodeWithAddr(CFG* cfg, Addr addr) {
	assert(cfg != 0);
	assert(addr != 0);

	CfgNode* node = cfg->nodeByAddr(addr);
	if (node == 0) {
		node = new CfgNode(CfgNode::CFG_PHANTOM);
		node->setData(new CfgNode::PhantomData(addr));
		cfg->addNode(node);
	}

	return node;
}

CfgNode* CFGReader::exitNode(CFG* cfg) {
	CfgNode* exit_node = cfg->exitNode();
	if (exit_node == 0) {
		exit_node = new CfgNode(CfgNode::CFG_EXIT);
		cfg->addNode(exit_node);
	}

	return exit_node;
}

CfgNode* CFGReader::haltNode(CFG* cfg) {
	CfgNode* halt_node = cfg->exitNode();
	if (halt_node == 0) {
		halt_node = new CfgNode(CfgNode::CFG_HALT);
		cfg->addNode(halt_node);
	}

	return halt_node;
}

void CFGReader::markIndirect(CfgNode* node) {
	assert(node->type() == CfgNode::CFG_BLOCK);
	CfgNode::BlockData* data =
		static_cast<CfgNode::BlockData*>(node->data());
	assert(data != 0);
	data->setIndirect(true);
}

void CFGReader::addCall(CfgNode* node, CFG* called, unsigned long long count) {
	assert(node->type() == CfgNode::CFG_BLOCK);
	CfgNode::BlockData* data =
		static_cast<CfgNode::BlockData*>(node->data());
	assert(data != 0);
	data->addCall(called);

	called->updateExecs(count);

	CfgNode* entry = called->entryNode();
	CfgNode* first = called->nodeByAddr(called->addr());
	if (entry && first) {
		CfgEdge* edge = called->findEdge(entry, first);
		assert(edge != 0);

		edge->updateCount(count);
	}
}

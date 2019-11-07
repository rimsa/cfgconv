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

#include <sstream>
#include <cassert>
#include <algorithm>

#include <CFG.h>
#include <CfgNode.h>
#include <CfgEdge.h>

CFG::CFG(Addr addr) : m_addr(addr), m_status(CFG::UNCHECKED),
		m_functionName("unknown"), m_complete(false),
		m_entryNode(0), m_exitNode(0), m_haltNode(0) {
}

CFG::~CFG() {
	for (CfgNode* node : m_nodes)
		delete node;

	for (CfgEdge* edge : m_edges)
		delete edge;
}

void CFG::setFunctionName(const std::string& functionName) {
	m_functionName = functionName;
}

bool CFG::complete() const {
	return (m_status == CFG::VALID ? m_complete : false);
}

CfgNode* CFG::nodeByAddr(Addr addr) const {
	std::map<Addr, CfgNode*>::const_iterator it = m_nodesMap.find(addr);
	return it != m_nodesMap.end() ? it->second : 0;
}

bool CFG::containsNode(CfgNode* node) const {
	return node ? (m_nodes.count(node) == 1) : false;
}

void CFG::addNode(CfgNode* node) {
	Addr addr;

	switch (node->type()) {
		case CfgNode::CFG_ENTRY:
			assert(m_entryNode == 0);
			m_entryNode = node;
			break;
		case CfgNode::CFG_BLOCK:
		case CfgNode::CFG_PHANTOM:
			addr = CfgNode::node2addr(node);
			assert(addr != 0);

			assert(m_nodesMap[addr] == 0);
			m_nodesMap[addr] = node;
			break;
		case CfgNode::CFG_EXIT:
			assert(m_exitNode == 0);
			m_exitNode = node;
			break;
		case CfgNode::CFG_HALT:
			assert(m_haltNode == 0);
			m_haltNode = node;
			break;
		default:
			assert(false);
	}

	m_nodes.insert(node);
	m_status = CFG::UNCHECKED;
}

bool CFG::containsEdge(CfgNode* src, CfgNode* dst) const {
	if (!src || !this->containsNode(src) ||
		!dst || !this->containsNode(dst))
		return false;

	std::map<CfgNode*, std::set<CfgNode*>>::const_iterator it = m_succs.find(src);
	return (it != m_succs.end() ? (it->second.count(dst) == 1) : false);
}

void CFG::addEdge(CfgNode* src, CfgNode* dst) {
	assert(src != 0 && this->containsNode(src));
	assert(dst != 0 && this->containsNode(dst));
	
	// This edge was already added.
	if (m_succs[src].count(dst) > 0)
		return;

	CfgEdge* edge = new CfgEdge(src, dst);
	m_edges.insert(edge);

	m_succs[src].insert(dst);
	m_preds[dst].insert(src);

	m_status = CFG::UNCHECKED;
}

static std::set<CfgNode*> emptyset;

const std::set<CfgNode*>& CFG::successors(CfgNode* node) const {
	assert(node != 0 && this->containsNode(node));

	std::map<CfgNode*, std::set<CfgNode*>>::const_iterator it = m_succs.find(node);
	return (it != m_succs.end() ? it->second : emptyset);
}

const std::set<CfgNode*>& CFG::predecessors(CfgNode* node) const {
	assert(node != 0 && this->containsNode(node));

	std::map<CfgNode*, std::set<CfgNode*>>::const_iterator it = m_preds.find(node);
	return (it != m_preds.end() ? it->second : emptyset);
}

enum CFG::Status CFG::check() {
	m_complete = true;
	m_status = CFG::INVALID;

	if (!m_entryNode || (!m_exitNode && !m_haltNode))
		goto out;

	for (CfgNode* node : this->nodes()) {
		CfgNode::BlockData* bdata;

		switch (node->type()) {
			case CfgNode::CFG_ENTRY:
				if (this->predecessors(node).size() != 0)
					goto out;
				else {
					const std::set<CfgNode*>& tmp = this->successors(node);
					if (tmp.size() == 0 || CfgNode::node2addr(*tmp.begin()) != this->addr())
						goto out;
				}

				break;
			case CfgNode::CFG_BLOCK:
				if (this->predecessors(node).size() == 0 ||
					this->successors(node).size() == 0)
					goto out;

				bdata = static_cast<CfgNode::BlockData*>(node->data());
				assert(bdata != 0);
				if (bdata->indirect())
					m_complete = false;

				break;
			case CfgNode::CFG_PHANTOM:
				if (this->predecessors(node).size() == 0 ||
					this->successors(node).size() != 0)
					goto out;

				assert(node->data() != 0);
				m_complete = false;

				break;
			case CfgNode::CFG_EXIT:
			case CfgNode::CFG_HALT:
				if (this->predecessors(node).size() == 0 ||
					this->successors(node).size() != 0)
					goto out;

				break;
			default:
				assert(false);
		}
	}

	m_status = CFG::VALID;

out:
	return m_status;
}

static
std::string dotFilter(const std::string& name) {
	std::stringstream ss;

	const char* str = name.c_str();
	while (*str) {
		if (*str == '<' || *str == '>')
			ss << "\\";

		ss << *str;
		str++;
	}

	return ss.str();
}

std::string CFG::toDOT() const {
	std::stringstream ss;
	int unknown = 1;

	ss << std::hex;
	ss << "digraph \"0x" << m_addr << "\" {" << std::endl;
	ss << "  label = \"0x" << m_addr << " (" << m_functionName << ")\"" << std::endl;
    ss << "  labelloc = \"t\"" << std::endl;
    ss << "  node[shape=record]" << std::endl;
    ss << std::endl;

	for (CfgNode* node : m_nodes) {
		switch (node->type()) {
			case CfgNode::CFG_ENTRY:
			    ss << "  Entry [label=\"\",width=0.3,height=0.3,shape=circle,fillcolor=black,style=filled]" << std::endl;
			    break;
			case CfgNode::CFG_EXIT:
				ss << "  Exit [label=\"\",width=0.3,height=0.3,shape=circle,fillcolor=black,style=filled,peripheries=2]" << std::endl;
			    break;
			case CfgNode::CFG_HALT:
				ss << "  Halt [label=\"\",width=0.3,height=0.3,shape=square,fillcolor=black,style=filled,peripheries=2]" << std::endl;
			    break;
			case CfgNode::CFG_BLOCK: {
				assert(node->data() != 0);
				CfgNode::BlockData* blockData = static_cast<CfgNode::BlockData*>(node->data());

				Addr addr =  blockData->addr();
				ss << "  \"0x" << addr << "\" [label=\"{" << std::endl;
				ss << "    0x" << addr << " [" << std::dec << blockData->size() << "]\\l" << std::endl;

				std::list<Instruction*> instrs = blockData->instructions();
				if (instrs.size() > 0) {
					ss << "    | [instrs]\\l" << std::endl;
					for (std::list<Instruction*>::const_iterator it = instrs.cbegin(), ed = instrs.cend();
							it != ed; it++) {
						Instruction* instr = *it;
						ss << "    &nbsp;&nbsp;0x" << std::hex << instr->addr() << " \\<+"
								<< std::dec << instr->size() << "\\>: " << dotFilter(instr->text())
								<< "\\l" << std::endl;
					}
				}

				std::set<CFG*> calls = blockData->calls();
				if (!calls.empty()) {
					ss << "     | [calls]\\l" << std::endl;
					ss << std::hex;
					for (std::set<CFG*>::const_iterator it = calls.cbegin(), ed = calls.cend();
							it != ed; it++) {
						CFG* called = *it;
						ss << "     &nbsp;&nbsp;0x" << called->addr() << " ("
							<< dotFilter(called->functionName()) << ")\\l" << std::endl;
					}
				}

				ss << "  }\"]" << std::endl;

                if (blockData->indirect()) {
                		ss << "  \"Unknown" << std::dec << unknown << "\" [label=\"?\", shape=none]" << std::endl;
                		ss << "  \"0x" << std::hex << blockData->addr() << "\" -> \"Unknown" << std::dec << unknown << "\" [style=dashed]" << std::endl;
                    unknown++;
                }

				ss << std::hex;
				break;
			}
			case CfgNode::CFG_PHANTOM: {
				assert(node->data() != 0);
				CfgNode::PhantomData* phantomData = static_cast<CfgNode::PhantomData*>(node->data());

				ss << "  \"0x" << phantomData->addr() << "\" [label=\"{" << std::endl;
				ss << "     0x" << phantomData->addr() << "\\l" << std::endl;
				ss << "  }\", style=dashed]" << std::endl;

				break;
			}
			default:
				assert(false);
		}
	}

	for (CfgEdge* edge : m_edges) {
		CfgNode* src = edge->source();
		switch (src->type()) {
			case CfgNode::CFG_ENTRY:
				ss << "  Entry -> ";
				break;
			case CfgNode::CFG_BLOCK:
			case CfgNode::CFG_PHANTOM:
				ss << "  \"0x" << CfgNode::node2addr(src) << "\" -> ";
				break;
			default:
				assert(false);
		}

		CfgNode* dst = edge->destination();
		switch (dst->type()) {
			case CfgNode::CFG_EXIT:
				ss << "Exit";
				break;
			case CfgNode::CFG_HALT:
				ss << "Halt";
				break;
			case CfgNode::CFG_BLOCK:
			case CfgNode::CFG_PHANTOM:
				ss << "\"0x" << CfgNode::node2addr(dst) << "\"";
				break;
			case CfgNode::CFG_ENTRY:
			default:
				assert(false);
		}

		ss << std::endl;
	}

	ss << "}" << std::endl;

	return ss.str();
}

void CFG::dumpDOT(const std::string& fileName) {
	std::ofstream fout(fileName);
	if (!fout.is_open())
		throw std::string("Unable to write file: ") + fileName;

	fout << this->toDOT();
	fout.close();
}

static
std::string node2name(CfgNode* node) {
	assert(node != 0);
	switch (node->type()) {
		case CfgNode::CFG_ENTRY:
			return "entry";
		case CfgNode::CFG_PHANTOM:
		case CfgNode::CFG_BLOCK: {
			std::stringstream ss;
			ss << std::hex << "0x" << CfgNode::node2addr(node);
			return ss.str();
			} break;
		case CfgNode::CFG_EXIT:
			return "exit";
		case CfgNode::CFG_HALT:
			return "halt";
		default:
			assert(false);
	}
	return "";
}

std::string CFG::str() const {
	std::stringstream ss;

	ss << std::hex << "[cfg 0x" << this->addr() << " \"" << this->functionName()
	   << "\" " << (this->complete() ? "true" : "false") << "]" << std::endl;
	for (CfgNode* node : this->nodes()) {
		// Only output block nodes.
		if (node->type() != CfgNode::CFG_BLOCK)
			continue;

		ss << "[node 0x" << this->addr() << " " << node2name(node);

		CfgNode::BlockData* data = static_cast<CfgNode::BlockData*>(node->data());
		assert(data != 0);

		ss << std::dec << " " << data->size();

		ss << " [";
		const std::list<Instruction*>& instrs = data->instructions();
		for (std::list<Instruction*>::const_iterator it = instrs.cbegin(),
				ed = instrs.cend(); it != ed; ++it) {
			if (it != instrs.cbegin())
				ss << " ";

			ss << (*it)->size();
		}
		ss << "]";

		ss << std::hex << " [";
		const std::set<CFG*>& calls = data->calls();
		for (std::set<CFG*>::const_iterator it = calls.cbegin(),
				ed = calls.cend(); it != ed; ++it) {
			if (it != calls.begin())
				ss << " ";

			ss << "0x" << (*it)->addr();
		}
		ss << "]";
	
		ss << " " << (data->indirect() ? "true" : "false");

		ss << " [";
		const std::set<CfgNode*>& succs = this->successors(node);
		for (std::set<CfgNode*>::const_iterator it = succs.cbegin(),
				ed = succs.cend(); it != ed; ++it) {
			if (it != succs.cbegin())
				ss << " ";

			ss << node2name(*it);
		}
		ss << "]]" << std::endl;
	}

	return ss.str();
}

std::ostream& operator<<(std::ostream& os, const CFG& cfg) {
	return os << cfg.str();
}

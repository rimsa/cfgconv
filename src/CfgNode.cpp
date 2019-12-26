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
#include <algorithm>

#include <CFG.h>
#include <CfgNode.h>
#include <Instruction.h>

CfgNode::CfgNode(enum CfgNode::Type type)
	: m_type(type), m_data(0) {
}

CfgNode::~CfgNode() {
	if (m_data)
		delete m_data;
}

void CfgNode::setData(CfgNode::Data* data) {
	assert(data != 0);

	PhantomData* phantomData = dynamic_cast<PhantomData*>(data);
	if (phantomData) {
		assert(m_data == 0);
		assert(m_type == CfgNode::CFG_PHANTOM);
		m_data = phantomData;
		return;
	}

	BlockData* blockData = dynamic_cast<BlockData*>(data);
	if (blockData) {
		switch (m_type) {
			case CfgNode::CFG_PHANTOM:
				assert(m_data != 0);
				delete m_data;

				m_type = CfgNode::CFG_BLOCK;
				m_data = blockData;
				return;
			case CfgNode::CFG_BLOCK:
				assert(m_data == 0);
				m_data = blockData;
				return;
			default:
				break;
		}
	}

	assert(false);
}

CfgNode::BlockData::BlockData(Addr addr, int size, bool indirect)
	: Data(addr), m_size(size), m_indirect(indirect) {
	int s = 0;
	while (s < size) {
		Instruction* i = Instruction::get(addr + s, 0);
		if (i->size() == 0)
			break;

		s += i->size();
		assert(s <= size);

		this->addInstruction(i);
	}
}

CfgNode::BlockData::~BlockData() {
	for (std::map<Addr, CfgCall*>::iterator it = m_calls.begin(),
			ed = m_calls.end(); it != ed; it++) {
		delete it->second;
	}

	for (std::map<int, CfgSignalHandler*>::iterator it = m_signalHandlers.begin(),
			ed = m_signalHandlers.end(); it != ed; it++) {
		delete it->second;
	}
}

void CfgNode::BlockData::setIndirect(bool indirect) {
	m_indirect = indirect;
}

void CfgNode::BlockData::addInstruction(Instruction* instr) {
	assert(instr != 0 && instr->size() > 0);

	if (m_instrs.empty()) {
		assert(instr->addr() == m_addr);
		m_instrs.push_back(instr);
	} else {
		Instruction* prev = m_instrs.back();
		assert(instr->addr() == (prev->addr() + prev->size()));
		m_instrs.push_back(instr);
	}

	int size = (instr->addr() + instr->size()) - m_addr;
	if (size > m_size)
		m_size = size;
}

void CfgNode::BlockData::addInstructions(const std::list<Instruction*>& instrs) {
	for (Instruction* instr : instrs)
		this->addInstruction(instr);
}

Instruction* CfgNode::BlockData::firstInstruction() const {
	return m_instrs.empty() ? 0: m_instrs.front();
}

Instruction* CfgNode::BlockData::lastInstruction() const {
	return m_instrs.empty() ? 0: m_instrs.back();
}

void CfgNode::BlockData::clearInstructions() {
	m_instrs.clear();
	m_size = 0;
}

std::set<CfgCall*> CfgNode::BlockData::calls() const {
	std::set<CfgCall*> cfgCalls;

	std::transform(m_calls.begin(), m_calls.end(),
			std::inserter(cfgCalls, cfgCalls.begin()),
			[](const std::map<Addr, CfgCall*>::value_type &pair) {
					return pair.second;
			}
	);

	return cfgCalls;
}

void CfgNode::BlockData::addCall(CFG* cfg, unsigned long long count) {
	Addr addr = cfg->addr();
	std::map<Addr, CfgCall*>::const_iterator it = m_calls.find(addr);
	if (it != m_calls.end()) {
		CfgCall* cfgCall = it->second;
		cfgCall->updateCount(count);
	} else {
		CfgCall* cfgCall = new CfgCall(cfg, count);
		m_calls[addr] = cfgCall;
	}
}

void CfgNode::BlockData::clearCalls() {
	m_calls.clear();
}

std::set<CfgSignalHandler*> CfgNode::BlockData::signalHandlers() const {
	std::set<CfgSignalHandler*> cfgSignalHandlers;

	std::transform(m_signalHandlers.begin(), m_signalHandlers.end(),
			std::inserter(cfgSignalHandlers, cfgSignalHandlers.begin()),
			[](const std::map<int, CfgSignalHandler*>::value_type &pair) {
					return pair.second;
			}
	);

	return cfgSignalHandlers;
}

void CfgNode::BlockData::addSignalHandler(int sigid, CFG* handler, unsigned long long count) {
	assert(sigid > 0);

	std::map<int, CfgSignalHandler*>::const_iterator it = m_signalHandlers.find(sigid);
	if (it != m_signalHandlers.end()) {
		CfgSignalHandler* cfgSignalHandler = it->second;
		assert(cfgSignalHandler->handler()->addr() == handler->addr());
		cfgSignalHandler->updateCount(count);
	} else {
		CfgSignalHandler* cfgSignalHandler = new CfgSignalHandler(sigid, handler, count);
		m_signalHandlers[sigid] = cfgSignalHandler;
	}
}

void CfgNode::BlockData::clearSignalHandlers() {
	m_signalHandlers.clear();
}

Addr CfgNode::node2addr(CfgNode* node) {
	switch (node->type()) {
		case CfgNode::CFG_BLOCK:
		case CfgNode::CFG_PHANTOM:
			assert(node->data() != 0);
			return node->data()->addr();
		default:
			return 0;
	}
}

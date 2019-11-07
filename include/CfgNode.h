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

#ifndef CFGNODE_H
#define CFGNODE_H

#include <set>
#include <list>
#include <Instruction.h>

class CFG;

class CfgNode {
public:
	enum Type {
		CFG_ENTRY,
		CFG_BLOCK,
		CFG_PHANTOM,
		CFG_EXIT,
		CFG_HALT
	};

	class Data {
	public:
		virtual ~Data() {}

		Addr addr() const { return m_addr; }

	protected:
		Addr m_addr;

		Data(Addr addr) : m_addr(addr) {}
	};

	class PhantomData : public Data {
	public:
		PhantomData(Addr addr) : Data(addr) {}
		virtual ~PhantomData() {};
	};

	class BlockData : public Data {
	public:
		BlockData(Addr addr, int size = 0, bool indirect = false)
			: Data(addr), m_size(size), m_indirect(indirect) {}
		virtual ~BlockData() {};

		int size() const { return m_size; }

		bool indirect() const { return m_indirect; }
		void setIndirect(bool indirect = true);

		const std::list<Instruction*>& instructions() const { return m_instrs; }
		void addInstruction(Instruction* instr);
		void addInstructions(const std::list<Instruction*>& instrs);
		Instruction* firstInstruction() const;
		Instruction* lastInstruction() const;
		void clearInstructions();

		const std::set<CFG*>& calls() const { return m_calls; }
		void addCall(CFG* cfg);
		void addCalls(const std::set<CFG*>& calls);
		void clearCalls();

	private:
		int m_size;
		bool m_indirect;
		std::list<Instruction*> m_instrs;
		std::set<CFG*> m_calls;
	
	};

	CfgNode(enum CfgNode::Type type);
	virtual ~CfgNode();

	enum CfgNode::Type type() const { return m_type; }
	Data* data() const { return m_data; }

	void setData(Data* data);

	static Addr node2addr(CfgNode* node);

private:
	enum CfgNode::Type m_type;
	Data* m_data;

};

#endif

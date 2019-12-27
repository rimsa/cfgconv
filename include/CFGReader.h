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

#ifndef CFG_READER_H
#define CFG_READER_H

#include <map>
#include <set>
#include <string>
#include <fstream>

#include <Addr.h>

class CFG;
class CfgNode;

class CFGReader {
public:
	virtual ~CFGReader();

	virtual void loadCFGs() = 0;

	std::set<CFG*> cfgs() const;
	CFG* cfg(Addr addr) const;

	CFG* instance(Addr addr);

protected:
	CFGReader(const std::string& filename);

	std::fstream m_input;
	std::map<Addr, CFG*> m_cfgs;

	static CfgNode* entryNode(CFG* cfg);
	static CfgNode* nodeWithAddr(CFG* cfg, Addr addr);
	static CfgNode* exitNode(CFG* cfg);
	static CfgNode* haltNode(CFG* cfg);
	static void markIndirect(CfgNode* node);
	static void addCall(CfgNode* node, CFG* called,
					unsigned long long count = 0, bool update = false);
	static void addSignalHandler(CfgNode* node, int sigid, CFG* sigHandler,
					unsigned long long count = 0, bool update = false);

};

#endif

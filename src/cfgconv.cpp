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

#include <iostream>
#include <set>
#include <string>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <getopt.h>

#include <CFG.h>
#include <BFTraceReader.h>
#include <CFGGrindReader.h>
#include <DCFGReader.h>
#include <Instruction.h>

struct Config {
	enum {
		UNDEF_TYPE,
		BFTRACE_TYPE,
		CFGGRIND_TYPE,
		DCFG_TYPE
	} type;

	enum {
		SHOW_ALL,
		SHOW_VALID_ONLY,
		SHOW_INVALID_ONLY
	} show;

	std::list<std::pair<Addr, Addr>> ranges;
	char* instrs;
	char* dump;
	char* input;
} config = { Config::UNDEF_TYPE, Config::SHOW_ALL,
				std::list<std::pair<Addr, Addr>>(), 0, 0, 0 };

inline std::string& ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
		std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

inline std::string& rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
		std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

inline std::string& trim(std::string &s) {
	return rtrim(ltrim(s));
}

void usage(char* progname) {
	std::cout << "Usage: " << progname << " <Options> [CFG file]" << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "   -t   type        Input type" << std::endl;
	std::cout << "                        bftrace: bftrace input format" << std::endl;
	std::cout << "                        cfggrind: cfggrind input format" << std::endl;
	std::cout << "                        dcfg: pinplay dcfg input format" << std::endl;
	std::cout << "   -s   show        Strategy to show CFGs" << std::endl;
	std::cout << "                        all: show all CFGs [default]" << std::endl;
	std::cout << "                        valid: show only valid CFGs" << std::endl;
	std::cout << "                        invalid: show only invalid CFGs" << std::endl;
	std::cout << "   -r   Range       Consider only CFGs in the range (start:end)" << std::endl;
	std::cout << "                        can be used multiple times" << std::endl;
	std::cout << "   -a   Addr        Consider only CFGs with given address" << std::endl;
	std::cout << "                        can be used multiple times" << std::endl;
	std::cout << "   -A   File        Load file with addresses, one address per line" << std::endl;
	std::cout << "                        can be used multiple times" << std::endl;
	std::cout << "   -i   File        Instructions map (address:size:assembly per entry) file" << std::endl;
	std::cout << "   -d   Directory   Dump DOT cfgs in directory" << std::endl;
	std::cout << std::endl;

	exit(1);
}

void readoptions(int argc, char* argv[]) {
	int opt;
	char* idx;
	Addr start, end;
	std::ifstream input;

	while ((opt = getopt(argc, argv, "t:s:r:a:A:i:d:")) != -1) {
		switch (opt) {
			case 't':
				if (strcasecmp(optarg, "bftrace") == 0)
					config.type = Config::BFTRACE_TYPE;
				else if (strcasecmp(optarg, "cfggrind") == 0)
					config.type = Config::CFGGRIND_TYPE;
				else if (strcasecmp(optarg, "dcfg") == 0)
					config.type = Config::DCFG_TYPE;
				else
					throw std::string("invalid type: ") + optarg;

				break;
			case 's':
				if (strcasecmp(optarg, "all") == 0)
					config.show = Config::SHOW_ALL;
				else if (strcasecmp(optarg, "valid") == 0)
					config.show = Config::SHOW_VALID_ONLY;
				else if (strcasecmp(optarg, "invalid") == 0)
					config.show = Config::SHOW_INVALID_ONLY;
				else
					throw std::string("invalid show: ") + optarg;

				break;
			case 'r':
				idx = strchr(optarg, ':');
				if (!idx)
					throw std::string("invalid range: ") + optarg;

				*idx = 0;
				idx++;

				if (strncasecmp(optarg, "0x", 2))
					optarg += 2;
				if (strncasecmp(idx, "0x", 2))
					idx += 2;

				start = std::stoul(optarg, 0, 16);
				end = std::stoul(idx, 0, 16);

				if (end < start) {
					std::stringstream ss;
					ss << std::hex;
					ss << "invalid range: " << end << " < " << start;

					throw ss.str();
				}

				config.ranges.push_back(std::make_pair(start, end));
				break;
			case 'a':
				if (strncasecmp(optarg, "0x", 2))
					optarg += 2;

				start = std::stoul(optarg, 0, 16);
				if (start != 0)
					config.ranges.push_back(std::make_pair(start, start));
				break;
			case 'A':
				input = std::ifstream(optarg);
				for (std::string line; getline(input, line); ) {
					trim(line);

					if (line.length() >= 2 && line.compare(0, 2, "0x"))
						line = line.substr(2);

					if (line.empty())
						continue;

					start = std::stoul(line, 0, 16);
					if (start != 0)
						config.ranges.push_back(std::make_pair(start, start));
				}
				input.close();

				break;
			case 'i':
				config.instrs = optarg;
				break;
			case 'd':
				config.dump = optarg;
				break;
			default:
				throw std::string("Invalid option: ") + (char) optopt;
		}
	}

	if (optind >= argc)
		usage(argv[0]);

	config.input = argv[optind++];

	if (optind < argc)
		throw std::string("Unknown extra option: ") + argv[optind];

	if (config.type == Config::UNDEF_TYPE)
		throw std::string("-t option is mandatory");
}

bool isAddrInRange(Addr addr) {
	// if the range list is empty, consider the address in range.
	if (config.ranges.size() == 0)
		return true;

	for (std::list<std::pair<Addr, Addr> >::const_iterator it =  config.ranges.cbegin(),
			ed = config.ranges.cend(); it != ed; it++) {
		if (addr >= it->first && addr <= it->second)
			return true;
	}

	return false;
}

int main(int argc, char* argv[]) {
	CFGReader* reader = 0;
	try {
		readoptions(argc, argv);

		if (config.instrs)
			Instruction::load(std::string(config.instrs));

		switch (config.type) {
			case Config::BFTRACE_TYPE:
				reader = new BFTraceReader(config.input);
				break;
			case Config::CFGGRIND_TYPE:
				reader = new CFGGrindReader(config.input);
				break;
			case Config::DCFG_TYPE:
				reader = new DCFGReader(config.input);
				break;
			default:
				assert(false);
		}

		reader->loadCFGs();
		for (CFG* cfg : reader->cfgs()) {
			if (!isAddrInRange(cfg->addr()))
				continue;

			bool show;
			switch (config.show) {
				case Config::SHOW_ALL:
					show = true;
					break;
				case Config::SHOW_VALID_ONLY:
					show = cfg->status() == CFG::VALID;
					break;
				case Config::SHOW_INVALID_ONLY:
					show = cfg->status() == CFG::INVALID;
					break;
				default:
					assert(false);
			}

			if (show) {
				std::cout << *cfg;

				if (config.dump) {
					std::stringstream ss;
					ss << config.dump << "/cfg-0x" << std::hex << cfg->addr() << ".dot";
					cfg->dumpDOT(ss.str());
				}
			}
		}
	} catch (const std::string& str) {
		std::cerr << "error: " << str << std::endl;
	}

	if (reader != 0)
		delete reader;

	if (config.instrs)
		Instruction::clear();

	return 0;
}

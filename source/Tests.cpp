#include "core/Scheduler.h"
#include <iostream>

namespace tt {
	static void showListEntry(cc::ListEntry* a) {
		std::cout << "a->_markedForDeletion: " << a->_markedForDeletion << std::endl;
		std::cout << "a->_paused: " << a->_paused << std::endl;
		std::cout << "a->_priority: " << static_cast<uint32_t>(a->_priority) << std::endl;
		std::cout << "a->_target: " << a->_target << std::endl;
		//std::cout << "a->_listEntries" << a->getLength()<<std::endl;
	}
	static void Test000_addressOfVector() {
		std::vector<std::string*> v1;
		v1.push_back(new std::string("Hellow"));
		std::vector<std::string*> v2 = v1;

		int i;
	}
	static void Test001() {

		cc::ListEntry::getFromPool(a);
		a = cc::ListEntry::get(nullptr, cc::Priority::LOW, false, false);
		cc::ListEntry* b = a;
		a = cc::ListEntry::get(nullptr, cc::Priority::LOW, false, false);
		delete a; delete b;
	}
}
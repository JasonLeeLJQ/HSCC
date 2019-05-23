/*********************
 * created on 2015.5.6
 ********************/

#include "NVM/NVMainFactory.h"
#include "include/Exception.h"
using namespace NVM;
NVMain* NVMainFactory::CreateNVMain( std::string nvm_type) 
{
	NVMain* main_mem = NULL;
	if( nvm_type == "NVMain") main_mem = new NVMain();
	if( nvm_type == "FineNVMain") main_mem = new FineNVMain();
	if( nvm_type == "HierDRAMCache")
	{
		std::cout<<"create hier dram cache"<<std::endl;
		main_mem = new HierDRAMCache();
	}
	if( nvm_type == "RBLANVMain" ) {
		std::cout<<"NVMainFactory::CreateNVMain--->正在创建RBLA_NVMain"<<std::endl;
		main_mem = new RBLA_NVMain();
	}
	if( nvm_type == "FLATNVMain") {
		std::cout<<"NVMainFactory::CreateNVMain--->正在创建FlatNVMain"<<std::endl;
		main_mem = new FlatNVMain();
	}
	if( nvm_type == "FlatRBLANVMain") {
		std::cout<<"NVMainFactory::CreateNVMain--->正在创建FlatRBLANVMain"<<std::endl;
		main_mem = new FlatRBLANVMain();
	}
	return main_mem;
}


/*依据nvm_type类型，创建NVMain对象*/
NVMain* NVMainFactory::CreateNewNVMain( std::string nvm_type)
{
	NVMain* main_mem = CreateNVMain(nvm_type);
	if( !main_mem )
	{
		NVM::Warning("didn't set main memory type! default set NVMain object as main memory");
		main_mem = CreateNVMain("NVMain");
	}
	return main_mem;
}

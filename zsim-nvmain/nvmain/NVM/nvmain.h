/*******************************************************************************
* Copyright (c) 2012-2014, The Microsystems Design Labratory (MDL)
* Department of Computer Science and Engineering, The Pennsylvania State University
* All rights reserved.
* 
* This source code is part of NVMain - A cycle accurate timing, bit accurate
* energy simulator for both volatile (e.g., DRAM) and non-volatile memory
* (e.g., PCRAM). The source code is free and you can redistribute and/or
* modify it by providing that the following conditions are met:
* 
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
* 
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
* Author list: 
*   Matt Poremba    ( Email: mrp5060 at psu dot edu 
*                     Website: http://www.cse.psu.edu/~poremba/ )
*******************************************************************************/

#ifndef __NVMAIN_H__
#define __NVMAIN_H__

#include <iostream>
#include <fstream>
#include <stdint.h>
#include "src/Params.h"
#include "src/NVMObject.h"
#include "src/Prefetcher.h"
#include "include/NVMainRequest.h"
#include "traceWriter/GenericTraceWriter.h"
#include "Decoders/MQMigrator/MQMigrator.h"
#include "include/NVMainConfig.h"
#include <stdarg.h>
#include "string.h"
#include "stdlib.h"

namespace NVM {

class Config;
class MemoryController;
class MemoryControllerManager;
class Interconnect;
class AddressTranslator;
class SimInterface;
class NVMainRequest;

class NVMain : public NVMObject
{
  public:
    NVMain( );
    ~NVMain( );

    virtual void SetConfig( Config *conf, std::string memoryName = "defaultMemory", bool createChildren = true );

    Config *GetConfig( );

    void IssuePrefetch( NVMainRequest *request );
    virtual bool IssueCommand( NVMainRequest *request );
    virtual bool IssueAtomic( NVMainRequest *request );
    bool IsIssuable( NVMainRequest *request, FailReason *reason=NULL );

    bool RequestComplete( NVMainRequest *request );

    bool CheckPrefetch( NVMainRequest *request );

    void RegisterStats( );
    void CalculateStats( );

    void Cycle( ncycle_t steps );
	virtual uint64_t GetMemorySize(){ return memory_size;}
	virtual unsigned GetMemoryWidth(){ return mem_width;}
	virtual unsigned GetWordSize(){ return word_size;}
	//has no implimentation of buffer , but provide interface function
	virtual uint64_t GetBufferSize(){ return 0; }
	virtual unsigned GetBufferWordSize(){ return 0; }
	virtual void SetMigrator( NVMObject* migrator){};
	virtual int GetChannels()
	{
		return numChannels;
	}
	uint64_t memory_size;
    ncounter_t totalReadRequests;
    ncounter_t totalWriteRequests;
	/* 每一个channel都对应一个内存控制器（MemoryController类）*/
    MemoryController **memoryControllers;
  protected:
	double GetSyncValue( );
	void CycleMemCtl( MemoryController** &mem_ctl , uint64_t num_channels);
	//全部的配置，如flat.config
	Config *config;
	//每个channel的配置，如NVM_channel.config & DRAM_channel.config
    Config **channelConfig;
    //MemoryController **memoryControllers;
    AddressTranslator *translator;
    SimInterface *simInterface;
    ncounter_t successfulPrefetches;
    ncounter_t unsuccessfulPrefetches;

    unsigned int numChannels;
	unsigned word_size;
	unsigned mem_width;
    double syncValue;

    Prefetcher *prefetcher;
    std::list<NVMainRequest *> prefetchBuffer;

    std::ofstream pretraceOutput;
    GenericTraceWriter *preTracer;  //write trace

    void PrintPreTrace( NVMainRequest *request );
    void GeneratePrefetches( NVMainRequest *request, std::vector<NVMAddress>& prefetchList );
};

};

////////////////////////////////////////////////////
//add by Jason Lee 2019,23,may
#define DEBUG_NVMAIN
inline void debug_NVMain( std::string format_str , ...)
{
	#ifdef DEBUG_NVMAIN
		format_str ="NVMain Test:" + format_str+"\n";
		va_list parg;
		va_start(parg , format_str);
		vfprintf(stdout , format_str.c_str() , parg);
		va_end(parg);
	#endif
}
/////////////////////////////////////////////////////
#endif

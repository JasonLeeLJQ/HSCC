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

#include "MemControl/FRFCFS/FRFCFS.h"
#include "src/EventQueue.h"
#include "include/NVMainRequest.h"

#include <iostream>
#include <set>
#include <assert.h>

using namespace NVM;

FRFCFS::FRFCFS( )
{
    std::cout << "Created a First Ready First Come First Serve memory controller!"
        << std::endl;

    queueSize = 32;
    starvationThreshold = 4;

    averageLatency = 0.0f;
    averageQueueLatency = 0.0f;
    averageTotalLatency = 0.0f;

    measuredLatencies = 0;
    measuredQueueLatencies = 0;
    measuredTotalLatencies = 0;

    mem_reads = 0;
    mem_writes = 0;

    rb_hits = 0;
    rb_miss = 0;
	rb_hit_rate = 0;	
    write_pauses = 0;
    starvation_precharges = 0;
    psInterval = 0;

    InitQueues( 1 );
    memQueue = &(transactionQueues[0]);
}

FRFCFS::~FRFCFS( )
{
    std::cout << "FRFCFS memory controller destroyed. " << memQueue->size( ) 
              << " commands still in memory queue." << std::endl;
}

void FRFCFS::SetConfig( Config *conf, bool createChildren )
{
    if( conf->KeyExists( "StarvationThreshold" ) )
    {
        starvationThreshold = static_cast<unsigned int>( conf->GetValue( "StarvationThreshold" ) );
    }

    if( conf->KeyExists( "QueueSize" ) )
    {
        queueSize = static_cast<unsigned int>( conf->GetValue( "QueueSize" ) );
    }
	
    MemoryController::SetConfig( conf, createChildren );

    SetDebugName( "FRFCFS", conf );
}

void FRFCFS::RegisterStats( )
{
    AddStat(mem_reads);
    AddStat(mem_writes);
    AddStat(rb_hits);
    AddStat(rb_miss);
    AddStat(rb_hit_rate);
    AddStat(starvation_precharges);
    AddStat(averageLatency);
    AddStat(averageQueueLatency);
    AddStat(averageTotalLatency);
    AddStat(measuredLatencies);
    AddStat(measuredQueueLatencies);
    AddStat(measuredTotalLatencies);
    AddStat(write_pauses);
    MemoryController::RegisterStats( );
}

/* 队列长度超出限制，不能发起request */
bool FRFCFS::IsIssuable( NVMainRequest * /*request*/, FailReason * /*fail*/ )
{
    bool rv = true;

    /*
     *  Limit the number of commands in the queue. This will stall the caches/CPU.
     */ 
    if( memQueue->size( ) >= queueSize )
    {
        rv = false;
		//std::cout<<"FlatNVMain can't issuable"<<std::endl;
    }
    return rv;
}

/*
 *  This method is called whenever a new transaction from the processor issued to
 *  this memory controller / channel. All scheduling decisions should be made here.
 *  每当从处理器发出一个新的事务给这个内存控制器/通道时，都会调用这个方法。所有的调度决策都应该在这里做出。
 */
bool FRFCFS::IssueCommand( NVMainRequest *req )
{
	//std::cout<<"nvmain: issue command to memory controller"<<std::endl;
	//std::cout<<"FineNvmain: memory controller issue"<<std::endl;
	//if( req->isFetch)
	//	std::cout<<"issue fetcher command: "<<std::hex<<req->address.GetPhysicalAddress()<<std::endl;
    if( !IsIssuable( req ) )
    {
        return false;
    }

    /* request命令到达channel控制器的时间 */
    req->arrivalCycle = GetEventQueue()->GetCurrentCycle();
    /* 
     *  Just push back the read/write. It's easier to inject dram commands than break it up
     *  here and attempt to remove them later.
     */
	//std::cout<<"nvmain: memory controller enqueue request , cycle is"<<req->arrivalCycle<<std::endl;
   //std::cout<<"#enqueue:"<<std::hex<<req->address.GetPhysicalAddress()<<std::endl;
    Enqueue( 0, req );
    if( req->type == READ || req->type==READ_PRECHARGE )
        mem_reads++;
    else if( req->type == WRITE || req->type == WRITE_PRECHARGE)
        mem_writes++;

    /*
     *  Return whether the request could be queued. Return false if the queue is full.
     */
    return true;
}


bool FRFCFS::RequestComplete( NVMainRequest * request )
{
	//std::cout<<"nvmain:FRFCFS request complete"<<std::endl;
	//if( request->type==REFRESH)
	//{
	//	std::cout<<"frfcfs:refresh"<<std::endl;
	//	ProcessRefreshPulse(request);
	//}
    if( request->type == WRITE || request->type == WRITE_PRECHARGE )
    {
        /* 
         *  Put cancelled requests at the head of the write queue
         *  like nothing ever happened.
         */
        if( request->flags & NVMainRequest::FLAG_CANCELLED 
            || request->flags & NVMainRequest::FLAG_PAUSED )
        {
            Prequeue( 0, request );

            return true;
        }
    }

    /* Only reads and writes are sent back to NVMain and checked for in the transaction queue. */
    if( request->type == READ 
        || request->type == READ_PRECHARGE 
        || request->type == WRITE 
        || request->type == WRITE_PRECHARGE )
    {
        request->status = MEM_REQUEST_COMPLETE;
        request->completionCycle = GetEventQueue()->GetCurrentCycle();

        /* Update the average latencies based on this request for READ/WRITE only. */
		//read/write time
        averageLatency = ((averageLatency * static_cast<double>(measuredLatencies))
                           + static_cast<double>(request->completionCycle)
                           - static_cast<double>(request->issueCycle))
                       / static_cast<double>(measuredLatencies+1);
        measuredLatencies += 1;
		//queue latency
        averageQueueLatency = ((averageQueueLatency * static_cast<double>(measuredQueueLatencies))
                                + static_cast<double>(request->issueCycle)
                                - static_cast<double>(request->arrivalCycle))
                            / static_cast<double>(measuredQueueLatencies+1);
        measuredQueueLatencies += 1;

        averageTotalLatency = ((averageTotalLatency * static_cast<double>(measuredTotalLatencies))
                                + static_cast<double>(request->completionCycle)
                                - static_cast<double>(request->arrivalCycle))
                            / static_cast<double>(measuredTotalLatencies+1);
        measuredTotalLatencies += 1;
	}
    return MemoryController::RequestComplete( request );
}


void FRFCFS::Cycle( ncycle_t steps )
{
    NVMainRequest *nextRequest = NULL;

    /* Check for starved requests BEFORE row buffer hits. */
    if( FindStarvedRequest( *memQueue, &nextRequest ) )
    {
        rb_miss++;
        starvation_precharges++;
    }
    /* Check for row buffer hits. */
    else if( FindRowBufferHit( *memQueue, &nextRequest) )
    {
		//nextRequest->rbHit = true;
        rb_hits++;
    }
    /* Check if the address is accessible through any other means. */
    else if( FindCachedAddress( *memQueue, &nextRequest ) )
    {
		rb_miss++;	
    }
    else if( FindWriteStalledRead( *memQueue, &nextRequest ) )
    {
		rb_miss++;
        if( nextRequest != NULL )
            write_pauses++;
    }
    /* Find the oldest request that can be issued. */
    else if( FindOldestReadyRequest( *memQueue, &nextRequest ) )
    {
        rb_miss++;
    }
    /* Find requests to a bank that is closed. */
    else if( FindClosedBankRequest( *memQueue, &nextRequest ) )
    {
        rb_miss++;
    }
    else
    {
        nextRequest = NULL;
    }

    /* Issue the commands for this transaction. */
    if( nextRequest != NULL )
    {
        IssueMemoryCommands( nextRequest );
    }

    /* Issue any commands in the command queues. */
    CycleCommandQueues( );
    MemoryController::Cycle( steps );
}

void FRFCFS::CalculateStats( )
{
	if( rb_hits+rb_miss!=0 )
	    rb_hit_rate = (double)rb_hits/(double)(rb_hits+rb_miss);
    MemoryController::CalculateStats( );
}


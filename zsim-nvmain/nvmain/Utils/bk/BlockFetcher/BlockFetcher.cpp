
#include "Utils/BlockFetcher/BlockFetcher.h"
#include "NVM/Fine_NVMain/FineNVMain.h"
#include "include/CommonMath.h"
#include "src/EventQueue.h"
#include "src/MemoryController.h"

using namespace NVM;
BlockFetcher::BlockFetcher():srcReq(NULL),dstReq(NULL),failed_cache_time(0),offset_shift(0),succeed_cache_time(0)
{
}

BlockFetcher::~BlockFetcher()
{

}

//init block fetcher according to config 
void BlockFetcher::Init( Config* conf)
{
	buffer_translator = dynamic_cast<BufferDecoder*>(((FineNVMain*)(parent->GetTrampoline()))->GetBufferDecoder());
	assert( buffer_translator);

	mem_col_size = buffer_translator->GetMemColBytes();
	buffer_col_size = buffer_translator->GetBufferColBytes();
	std::cout<<"mem_col_size is: "<<mem_col_size<<std::endl;
	std::cout<<"buffer_col_size is: "<<buffer_col_size<<std::endl;
	assert( mem_col_size>0);
	assert( buffer_col_size>0);
	RegisterStats();
}

/**
 * @ function:
 * @ param:
 * @ return:
 **/
bool BlockFetcher::IssueAtomic(NVMainRequest* req)
{
	return true;
}

/**
 * @ function:
 * @ param:
 * @ return:
 **/
bool BlockFetcher::IssueCommand( NVMainRequest *req)
{
	uint64_t data_size = req->burstCount;
	bool src_is_buffer = req->address.IsBufferAddr();
	bool dst_is_buffer = req->address.DestAddrIsBuffer();
	//std::cout<<"begin buffer data block"<<std::endl;
	if( !buffer_translator->isCaching() )
	{
		Fetch( req , data_size , src_is_buffer , dst_is_buffer );
	}
	else
	{
		//Fetch(req,data_size,src_is_buffer, dst_is_buffer);
		failed_list.push_back( req );
		failed_cache_time++;
		return false;
	}
	/*while( !req_list.empty())
	{
		std::cout<<"size of request list:"<<req_list.size()<<std::endl;
		if( !buffer_translator->isCaching() )
		{
			NVMainRequest* head_req = req_list.front();
			req_list.pop_front();
			uint64_t data_size = head_req->burstCount;
			bool src_is_buffer = head_req->address.IsBufferAddr();
			bool dst_is_buffer = head_req->address.DestAddrIsBuffer();
			Fetch( head_req , data_size , src_is_buffer , dst_is_buffer );
		}
	}*/
		//std::cout<<"failed cache : "<<failed_cache_time<<std::endl;
		//failed_cache_time++;
	return true;
}

/**
 * @ function:
 * @ param:
 * @ return:
 **/
void BlockFetcher::Cycle( ncycle_t steps)
{

}

/**
 * @ function:
 * @ param:
 * @ return:
 **/
bool BlockFetcher::CheckIssuable( NVMAddress &address , OpType type )
{
	NVMainRequest req;
	req.address = address;
	req.type = type;
	//check whether DRAM Buffer can issue command
	uint64_t row,col,bank,rank,channel,subarray;
	uint64_t child_index;
	if( address.IsBufferAddr() )
	{
		buffer_translator->Translate(address.GetPhysicalAddress(), &row , &col, &bank, &rank, &channel , &subarray);
		child_index = channel + FineNVMain::numChannels; 
		//std::cout<<"translate dram buffer addr:"<<std::hex<<address.GetPhysicalAddress()<<" child index is:"<<child_index<<std::endl;
	}
	else
	{
		//std::cout<<"translate main memory address:"<<std::hex<<address.GetPhysicalAddress()<<std::endl;
		(parent->GetTrampoline()->GetDecoder())->Translate(address.GetPhysicalAddress(), &row , &col, &bank, &rank, &channel , &subarray);
		child_index = channel;
		//std::cout<<"translate main memory address:"<<std::hex<<address.GetPhysicalAddress()<<" channel is"<<child_index<<std::endl;
	}
	address.SetTranslatedAddress(row,col,bank,rank,channel,subarray);
	//check whether main memory can issue command
	bool issuable = parent->GetTrampoline()->GetChild(child_index)->IsIssuable(&req);
	return issuable;
}

/**
 * @ function: fetch data from src_addr to dst_addr(dram buffer)
 * @ param:
 * @ return:
 **/
bool BlockFetcher::Fetch( NVMainRequest* request , uint64_t data_size , bool src_is_buffer , bool dst_is_buffer )
{
	srcNVMAddress.SetPhysicalAddress(request->address.GetPhysicalAddress());
	srcNVMAddress.SetAddrBuffer(src_is_buffer);
	dstNVMAddress.SetAddrBuffer( dst_is_buffer );
	dstNVMAddress.SetPhysicalAddress(request->address.GetDestAddress());
	bool ret = false;
	//read from src block
	//write to dest block
	if( CheckIssuable(srcNVMAddress,READ) && CheckIssuable(dstNVMAddress,WRITE) )
	{
		offset_shift = NVM::Log2(data_size);
		buffer_translator->StartBuffer(srcNVMAddress , dstNVMAddress , offset_shift);
		/**-----calculate source and dst burst count----**/
		unsigned src_burst_count = 0;
		unsigned dst_burst_count = 0;
		if( src_is_buffer )
			src_burst_count = data_size/buffer_col_size;
		else
			src_burst_count = data_size/mem_col_size;
		if( dst_is_buffer)
			dst_burst_count = data_size/buffer_col_size;
		else
			dst_burst_count = data_size/mem_col_size;
		//construct source read request	
		srcReq = new NVMainRequest();
		dstReq = new NVMainRequest();
		srcReq->address = srcNVMAddress;
		srcReq->type = READ;
		//how many times read data from memory read
		srcReq->burstCount = src_burst_count;
		srcReq->owner= parent->GetTrampoline();
		srcReq->isFetch = true;
		srcReq->issueCycle = GetEventQueue()->GetCurrentCycle();

		//construct dst write request
		dstReq->address = dstNVMAddress;
		dstReq->type = WRITE;
		dstReq->burstCount = dst_burst_count;
		dstReq->owner = parent->GetTrampoline();
		dstReq->isFetch = true;
		dstReq->issueCycle = GetEventQueue()->GetCurrentCycle();
		//std::cout<<"issue command of buffering through parent , src: "<<std::hex<<srcNVMAddress.GetPhysicalAddress()<<" dst:"<<std::hex<<dstNVMAddress.GetPhysicalAddress()<<std::endl;
		//issue command
		parent->GetTrampoline()->IssueCommand(srcReq);
		parent->GetTrampoline()->IssueCommand(dstReq);
		delete request;
		ret = true;
	}
	return ret;
}


/**
 * @ function:
 * @ param:
 * @ return:
 **/
bool BlockFetcher::RequestComplete( NVMainRequest* req)
{
	//std::cout<<"blockfetcher: request complete"<<std::endl;
	bool completed = buffer_translator->SetOpComplete(req->address , offset_shift);
	if( completed)
	{	
		if( !failed_list.empty())
		{
			//std::cout<<"has "<<failed_list.size()<<" faile requests"<<std::endl;
			//uint64_t current_cycle = GetEventQueue()->GetCurrentCycle();
			//uint64_t buffer_latency = GetEventQueue()->GetCurrentCycle()-req->issueCycle;
			//std::cout<<std::dec<<"buffer_latency "<<buffer_latency<<"after add:"<<GetEventQueue()->GetCurrentCycle()<<std::endl;
			/*std::cout<<"buffer_latency "<<buffer_latency<<" before add:"<<current_cycle<<std::endl;
			GetEventQueue()->SetCurrentCycle( current_cycle+ failed_list.size()*buffer_latency);
			std::cout<<"buffer_latency "<<buffer_latency<<"after add:"<<GetEventQueue()->GetCurrentCycle()<<std::endl;
			succeed_cache_time = succeed_cache_time+failed_list.size()+1; */
				NVMainRequest* tmp_req = failed_list.front();
				failed_list.pop_front();
				if(IssueCommand(tmp_req))
				{
					failed_cache_time--;
					//std::cout<<"reissue command succeed"<<std::endl;
				}
		}
		succeed_cache_time++;
		//std::cout<<"succeed cache time "<<succeed_cache_time<<std::endl;
	}
	delete req; 
	return true;
}

void BlockFetcher::RegisterStats()
{
	AddStat(succeed_cache_time);
	AddStat(failed_cache_time);
}
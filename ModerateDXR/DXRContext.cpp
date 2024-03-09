#include "DxrContext.hpp"
DxrContext::DxrContext(uint initWidth, uint initHeight)
{

}

DxrContext::~DxrContext()
{
}
void DxrContext::Flush()
{
	static UINT64 value = 1;
	commandQueue->Signal(fence.Get(), value);
	fence->SetEventOnCompletion(value++, nullptr);
}
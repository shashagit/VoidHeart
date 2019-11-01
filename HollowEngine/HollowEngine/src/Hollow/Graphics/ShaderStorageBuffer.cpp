#include <hollowpch.h>
#include "ShaderStorageBuffer.h"

#include "Utils/GLCall.h"

namespace Hollow
{
	ShaderStorageBuffer::ShaderStorageBuffer() : mRendererID(0), mSize(0)
	{
		GLCall(glGenBuffers(1, &mRendererID));
	}

	ShaderStorageBuffer::~ShaderStorageBuffer()
	{
		GLCall(glDeleteBuffers(1, &mRendererID));
	}

	void ShaderStorageBuffer::Bind(unsigned int bindPoint) const
	{
		GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, mRendererID));
		GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindPoint, mRendererID));
	}

	void ShaderStorageBuffer::Unbind(unsigned int bindPoint) const
	{
		GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, mRendererID));
		GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindPoint, 0));
	}

	void ShaderStorageBuffer::CreateBuffer(unsigned int size)
	{
		GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, mRendererID));
		mSize = size;
		GLCall(glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, NULL, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT));
		//GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, size, NULL, GL_STATIC_DRAW));
	}

	void ShaderStorageBuffer::ReleaseBufferPointer() const
	{
		GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, mRendererID));
		GLCall(glUnmapBuffer(GL_SHADER_STORAGE_BUFFER));
	}

	void ShaderStorageBuffer::PutMemoryBarrier()
	{
		GLCall(glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT));
	}

	void* ShaderStorageBuffer::GetBufferWritePointer(const bool invalidateOldData) const
	{
		GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, mRendererID));
		GLbitfield flag = GL_MAP_WRITE_BIT;
		if (invalidateOldData) flag |= GL_MAP_INVALIDATE_BUFFER_BIT;
		return glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, mSize, flag);
	}

	void* ShaderStorageBuffer::GetBufferReadPointer() const
	{
		GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, mRendererID));
		return glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, mSize, GL_MAP_READ_BIT);
	}

	void* ShaderStorageBuffer::GetBufferReadWritePointer() const
	{
		GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, mRendererID));
		return glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, mSize, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
	}

}
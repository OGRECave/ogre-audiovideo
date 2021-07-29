/**
* @file LocklessQueue.h
* @author  Ian Stangoe
* @version 1.14
*
* @section LICENSE
* 
* This source file is part of OgreOggSound, an OpenAL wrapper library for   
* use with the Ogre Rendering Engine.										 
*                                                                           
* Copyright 2010 Ian Stangoe 
*                                                                           
* OgreOggSound is free software: you can redistribute it and/or modify		  
* it under the terms of the GNU Lesser General Public License as published	 
* by the Free Software Foundation, either version 3 of the License, or		 
* (at your option) any later version.										 
*																			 
* OgreOggSound is distributed in the hope that it will be useful,			 
* but WITHOUT ANY WARRANTY; without even the implied warranty of			 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the			 
* GNU Lesser General Public License for more details.						 
*																			 
* You should have received a copy of the GNU Lesser General Public License	 
* along with OgreOggSound.  If not, see <http://www.gnu.org/licenses/>.	 
*
* @section DESCRIPTION
* 
* Template class for a lockless queue system to pass items from one thread to another.
* Only 1 thread can push and 1 thread can pop it. 
* All credit goes to: Lf3THn4D
*/

namespace OgreOggSound
{
	//! LocklessQueue template: as provided by Lf3THn4D
	/*!
	* \tparam Type Object type to queue.
	* This is a lockless queue system to pass
	* items from one thread to another.
	* \note
	* Only 1 thread can push and 1 thread can pop it.
	*/
	template <class Type>
	class LocklessQueue
	{
	private:
		//! Buffer to keep the queue.
		Type* m_buffer;

		//! Head of queue list.
		size_t m_head;

		//! Tail of queue list.
		size_t m_tail;

		//! Size of buffer.
		size_t m_size;

	public:
		//! Constructor.
		inline LocklessQueue(size_t size) :
		m_head(0), m_tail(0), m_size(size + 1)
		{
			m_buffer = new Type[m_size];
		}

		//! Destructor.
		inline ~LocklessQueue()
		{
			delete [] m_buffer;
		}

		//! Push object into the queue.
		inline bool push(const Type& obj)
		{
			size_t next_head = (m_head + 1) % m_size;
			if (next_head == m_tail) return false;
			m_buffer[m_head] = obj;
			m_head = next_head;
			return true;
		}

		//! Query status.
		/**
		@remarks
			Added query function for quickly testing status (Ian Stangoe)
		*/
		inline bool empty() const
		{
			return (m_head==m_tail);
		}

		//! Pop object out from the queue.
		inline bool pop(Type& obj)
		{
			if (m_tail == m_head) return false;
			obj = m_buffer[m_tail];
			m_tail = (m_tail + 1) % m_size;
			return true;
		}
	};
};

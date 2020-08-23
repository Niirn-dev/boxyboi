#pragma once
#include "Box.h"
#include <unordered_map>
#include <functional>
#include <typeinfo>
#include <cassert>

using TraitPair = std::pair<
	const std::type_info*,
	const std::type_info*
>;

namespace std
{
	template<>
	struct hash<TraitPair>
	{
		size_t operator()( const TraitPair& tp ) const
		{
			auto seed = tp.first->hash_code();
			seed ^= tp.second->hash_code() + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
			return seed;
		}
	};

	template<>
	struct equal_to<TraitPair>
	{
		bool operator()( const TraitPair& lhs,const TraitPair& rhs ) const
		{
			return lhs.first == rhs.first && lhs.second == rhs.second;
		}
	};
}

class PatternMatchingListener : public b2ContactListener
{
public:
	template<class T,class U,class F>
	void Case( F f )
	{
		handlers[{ &typeid( T ),&typeid( U ) }] = f;
		handlers[{ &typeid( U ),&typeid( T ) }] = std::bind(
			f,std::placeholders::_2,std::placeholders::_1
		);
	}
	template<class T,class U>
	bool HasCase()
	{
		if ( handlers.find( { &typeid( T ),&typeid( U ) } ) != handlers.end() )
		{
			return true;
		}
		return false;
	}
	template<class T,class U>
	void ClearCase()
	{
		handlers.erase( { &typeid( T ),&typeid( U ) } );
		handlers.erase( { &typeid( U ),&typeid( T ) } );
	}
	template<class F>
	void Default( F f )
	{
		dflt = f;
	}

	void BeginContact( b2Contact* contact ) override
	{
		const b2Body* bodyPtrs[] = { contact->GetFixtureA()->GetBody(),contact->GetFixtureB()->GetBody() };
		if ( bodyPtrs[0]->GetType() == b2BodyType::b2_dynamicBody &&
			 bodyPtrs[1]->GetType() == b2BodyType::b2_dynamicBody )
		{
			Box* boxPtrs[] = {
				reinterpret_cast<Box*>( bodyPtrs[0]->GetUserData() ),
				reinterpret_cast<Box*>( bodyPtrs[1]->GetUserData() )
			};

			Switch( boxPtrs[0],boxPtrs[1] );
		}
	}


private:
	void Switch( Box* a,Box* b )
	{
		if ( auto t = handlers.find( {
				&typeid( a->GetColorTrait() ),
				&typeid( b->GetColorTrait() )
			} ); t != handlers.end() )
		{
			t->second( a,b );
		}
		else
		{
			dflt( a,b );
		}
	}

private:
	std::unordered_map<TraitPair,std::function<void( Box*,Box* )>> handlers;
	std::function<void( Box*,Box* )> dflt = []( Box*,Box* ) {};
};

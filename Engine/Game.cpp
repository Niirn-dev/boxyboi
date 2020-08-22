/******************************************************************************************
*	Chili DirectX Framework Version 16.10.01											  *
*	Game.cpp																			  *
*	Copyright 2016 PlanetChili.net <http://www.planetchili.net>							  *
*																						  *
*	This file is part of The Chili DirectX Framework.									  *
*																						  *
*	The Chili DirectX Framework is free software: you can redistribute it and/or modify	  *
*	it under the terms of the GNU General Public License as published by				  *
*	the Free Software Foundation, either version 3 of the License, or					  *
*	(at your option) any later version.													  *
*																						  *
*	The Chili DirectX Framework is distributed in the hope that it will be useful,		  *
*	but WITHOUT ANY WARRANTY; without even the implied warranty of						  *
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the						  *
*	GNU General Public License for more details.										  *
*																						  *
*	You should have received a copy of the GNU General Public License					  *
*	along with The Chili DirectX Framework.  If not, see <http://www.gnu.org/licenses/>.  *
******************************************************************************************/
#include "MainWindow.h"
#include "Game.h"
#include "Box.h"
#include "Mat2.h"
#include <algorithm>
#include <sstream>
#include <typeinfo>
#include <functional>
#include <iterator>
#include <set>

std::set<Box*> upForPartition;

Game::Game( MainWindow& wnd )
	:
	wnd( wnd ),
	gfx( wnd ),
	world( { 0.0f,-0.5f } ),
	pepe( gfx )
{
	pepe.effect.vs.cam.SetPos( { 0.0,0.0f } );
	pepe.effect.vs.cam.SetZoom( 1.0f / boundarySize );

	std::generate_n( std::back_inserter( boxPtrs ),nBoxes,[this]() {
		return Box::Spawn( boxSize,bounds,world,rng );
	} );
	
	em.Case( { Colors::Blue,Colors::Green } ) = 
		[&]( const std::pair<Box*,Box*> bp ) 
	{ 
		SplitSmallest( bp );
	};
	em.Case( { Colors::Blue,Colors::White } ) =
		[]( const std::pair<Box*,Box*> bp )
	{
		if ( bp.first->GetColorTrait().GetColor() == Colors::White )
		{
			bp.first->SetColorTrait( std::move( bp.second->GetColorTrait().Clone() ) );
		}
		else
		{
			bp.second->SetColorTrait( std::move( bp.first->GetColorTrait().Clone() ) );
		}
	};

	class Listener : public b2ContactListener
	{
	public:
		void BeginContact( b2Contact* contact ) override
		{
			const b2Body* bodyPtrs[] = { contact->GetFixtureA()->GetBody(),contact->GetFixtureB()->GetBody() };
			if( bodyPtrs[0]->GetType() == b2BodyType::b2_dynamicBody &&
				bodyPtrs[1]->GetType() == b2BodyType::b2_dynamicBody )
			{
				Box* boxPtrs[] = { 
					reinterpret_cast<Box*>(bodyPtrs[0]->GetUserData()),
					reinterpret_cast<Box*>(bodyPtrs[1]->GetUserData())
				};
				auto& tid0 = typeid(boxPtrs[0]->GetColorTrait());
				auto& tid1 = typeid(boxPtrs[1]->GetColorTrait());

				std::stringstream msg;
				msg << "Collision between " << tid0.name() << " and " << tid1.name() << std::endl;
				OutputDebugStringA( msg.str().c_str() );
			}
		}
		void EndContact( b2Contact* contact ) override
		{
			const b2Body* bodyPtrs[] = { contact->GetFixtureA()->GetBody(),contact->GetFixtureB()->GetBody() };
			if ( bodyPtrs[0]->GetType() == b2BodyType::b2_dynamicBody &&
				 bodyPtrs[1]->GetType() == b2BodyType::b2_dynamicBody )
			{
				Box* boxPtrs[] = {
					reinterpret_cast<Box*>( bodyPtrs[0]->GetUserData() ),
					reinterpret_cast<Box*>( bodyPtrs[1]->GetUserData() )
				};
				auto& tid0 = typeid( boxPtrs[0]->GetColorTrait() );
				auto& tid1 = typeid( boxPtrs[1]->GetColorTrait() );

				std::stringstream msg;
				msg << "End of collision between " << tid0.name() << " and " << tid1.name() << std::endl;
				OutputDebugStringA( msg.str().c_str() );

				const auto& c0 = boxPtrs[0]->GetColorTrait().GetColor();
				const auto& c1 = boxPtrs[1]->GetColorTrait().GetColor();

				if ( ( ( c0 == Colors::Green ) && ( c1 == Colors::Blue ) ) ||
					 ( ( c0 == Colors::Blue ) && ( c1 == Colors::Green ) ) ||
					 ( ( c0 == Colors::Blue ) && ( c1 == Colors::White ) ) ||
					 ( ( c0 == Colors::White ) && ( c1 == Colors::Blue ) ) )
				{
					if ( upForPartition.find( boxPtrs[0] ) == upForPartition.end() &&
						 upForPartition.find( boxPtrs[1] ) == upForPartition.end() )
					{
						upForPartition.emplace( boxPtrs[0] );
						upForPartition.emplace( boxPtrs[1] );
					}
				}
			}
		}
	};
	static Listener mrLister;
	world.SetContactListener( &mrLister );
}

void Game::Go()
{
	gfx.BeginFrame();
	UpdateModel();
	ComposeFrame();
	gfx.EndFrame();
}

void Game::UpdateModel()
{
	const float dt = ft.Mark();
	world.Step( dt,8,3 );

	if ( !world.IsLocked() )
	{
		if ( !upForPartition.empty() )
		{
			for ( auto it = upForPartition.begin(); it != upForPartition.end(); std::advance( it,1 ) )
			{
				auto prev = it;
				std::advance( it,1 );
				em.Call( { *prev,*it } );
			}
			upForPartition.clear();
		}
	}
}

bool Game::SplitBox( const Box* box,unsigned int factor/*= 2*/ )
{
	const float oldSize = box->GetSize();
	const float newSize = oldSize / (float)factor;
	if ( newSize < minBoxSize )
	{
		return false;
	}

	const Vec2 oldBoxCenter = box->GetPosition();
	const Mat2 rMat = _Mat2<float>::Rotation( box->GetAngle() );
	const Vec2 oldBottomLeft = oldBoxCenter - Vec2{ oldSize,oldSize };
	const Vec2 oldTopRight = oldBoxCenter + Vec2{ oldSize,oldSize };
	for ( Vec2 pos = oldBottomLeft + Vec2{ newSize,newSize };
		  pos.y < oldTopRight.y; pos.y += 2 * newSize )
	{
		for ( pos.x = oldBottomLeft.x; pos.x < oldTopRight.x; pos.x += 2 * newSize )
		{
			const Vec2 toRotatedPos = ( pos - oldBoxCenter ) * rMat;
			boxPtrs.push_back( std::make_unique<Box>(
					std::move( box->GetColorTrait().Clone() ),
					world,
					oldBoxCenter + toRotatedPos,
					newSize,
					box->GetAngle(),
					box->GetVelocity(),
					box->GetAngularVelocity()
				) );
		}
	}
	return true;
}

void Game::DestroyBox( const Box* box )
{
	auto target = std::find_if( boxPtrs.begin(),boxPtrs.end(),
								[&box]( const std::unique_ptr<Box>& p )
								{
									return p.get() == box;
								} );
	target->reset();
	boxPtrs.erase( target );
}

void Game::SplitSmallest( const std::pair<Box*,Box*>& bp )
{
	if ( bp.first->GetSize() > bp.second->GetSize() )
	{
		if ( SplitBox( bp.first ) )
		{
			DestroyBox( bp.first );
		}
	}
	else
	{
		if ( SplitBox( bp.second ) )
		{
			DestroyBox( bp.second );
		}
	}
}

void Game::ComposeFrame()
{
	for( const auto& p : boxPtrs )
	{
		p->Draw( pepe );
	}
}
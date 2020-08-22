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
		[&]( const std::pair<Box*,Box*>& bp ) 
	{ 
		SplitSmallest( bp );
	};
	em.Case( { Colors::Blue,Colors::White } ) =
		[]( const std::pair<Box*,Box*>& bp )
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
	em.Case( { Colors::Yellow,Colors::Blue } ) =
		[&]( const std::pair<Box*,Box*>& bp )
	{
		if ( bp.first->GetColorTrait().GetColor() == Colors::Blue )
		{
			DestroyBox( bp.first );
		}
		else
		{
			DestroyBox( bp.second );
		}
	};
	em.Case( { Colors::Red,Colors::Yellow } ) =
		[]( const std::pair<Box*,Box*>& bp )
	{
		bp.first->ApplyLinearImpulse( bp.first->GetVelocity() * 0.5f );
		bp.second->ApplyLinearImpulse( bp.second->GetVelocity() * 0.5f );
	};
	em.Case( { Colors::Cyan,Colors::Cyan } ) =
		[&]( const std::pair<Box*,Box*>& bp )
	{
		world.SetGravity( -world.GetGravity() );
	};

	class Listener : public b2ContactListener
	{
	public:
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

				if ( upForPartition.find( boxPtrs[0] ) == upForPartition.end() &&
					 upForPartition.find( boxPtrs[1] ) == upForPartition.end() )
				{
					upForPartition.emplace( boxPtrs[0] );
					upForPartition.emplace( boxPtrs[1] );
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
	for ( auto [pos,i] = std::pair( oldBottomLeft + Vec2{ newSize,newSize },0 );
		  pos.y < oldTopRight.y && i < factor; pos.y += 2 * newSize,++i )
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
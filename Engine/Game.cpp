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
#include "PatternMatchingListener.h"
#include "ColorTrait.h"
#include <algorithm>
#include <sstream>
#include <typeinfo>
#include <functional>
#include <iterator>
#include <set>

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

	static PatternMatchingListener mrLister;

	mrLister.Case<BlueTrait,GreenTrait>( [&]( Box* a,Box* b )
										 {
											 SplitSmallest( a,b );
										 } );
	mrLister.Case<BlueTrait,WhiteTrait>( []( Box* a,Box* b )
										 {
											 if ( a->GetColorTrait().GetColor() == Colors::White )
											 {
												 a->SetColorTrait( std::move( b->GetColorTrait().Clone() ) );
											 }
											 else
											 {
												 b->SetColorTrait( std::move( a->GetColorTrait().Clone() ) );
											 }
										 } );
	mrLister.Case<YellowTrait,BlueTrait>( [&]( Box* a,Box* b )
										  {
											  if ( a->GetColorTrait().GetColor() == Colors::Blue )
											  {
												  DestroyBox( a );
											  }
											  else
											  {
												  DestroyBox( b );
											  }
										  } );
	mrLister.Case<RedTrait,YellowTrait>( []( Box* a,Box* b )
										 {
											 a->ApplyLinearImpulse( a->GetVelocity() * 0.5f );
											 b->ApplyLinearImpulse( b->GetVelocity() * 0.5f );
										 } );
	mrLister.Case<CyanTrait,CyanTrait>( [&]( Box* a,Box* b )
										{
											world.SetGravity( -world.GetGravity() );
										} );

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
		// Destroy boxes marked for death
		boxPtrs.erase(
			std::remove_if( boxPtrs.begin(),boxPtrs.end(),std::mem_fn( &Box::IsMarkedForDeath ) ),
			boxPtrs.end()
		);
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
	for ( auto [pos,i] = std::pair( oldBottomLeft + Vec2{ newSize,newSize },0u );
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

void Game::DestroyBox( Box* box )
{
	box->MarkForDeath();
}

void Game::SplitSmallest( Box* a,Box* b )
{
	if ( a->GetSize() > b->GetSize() )
	{
		if ( SplitBox( a ) )
		{
			DestroyBox( a );
		}
	}
	else
	{
		if ( SplitBox( b ) )
		{
			DestroyBox( b );
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
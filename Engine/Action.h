#pragma once
#include "Box.h"

#include <iterator>
#include <vector>
#include <memory>

class Action
{
public:
	virtual ~Action() = default;
	virtual void Do( std::vector<std::unique_ptr<Box>>& boxes,b2World& world ) = 0;
};

class Destroy : public Action
{
public:
	Destroy( Box* target )
		:
		target( target )
	{
	}
	void Do( std::vector<std::unique_ptr<Box>>& boxes,b2World& world ) override
	{
		target->MarkForDeath();
	}

private:
	Box* target;
};

class Split : public Action
{
public:
	Split( Box* target )
		:
		target( target )
	{
	}
	void Do( std::vector<std::unique_ptr<Box>>& boxes,b2World& world ) override
	{
		if ( target->GetSize() / 2.0f >= Box::MinSize() )
		{
			auto children = target->Split( world );
			boxes.insert(
				boxes.end(),
				std::make_move_iterator( children.begin() ),
				std::make_move_iterator( children.end() )
			);
		}
	}

private:
	Box* target;
};

class Tag : public Action
{
public:
	Tag( Box* target,std::unique_ptr<Box::ColorTrait> pct )
		:
		target( target ),
		pColorTrait( std::move( pct ) )
	{
	}
	void Do( std::vector<std::unique_ptr<Box>>& boxes,b2World& world ) override
	{
		target->SetColorTrait( std::move( pColorTrait ) );
	}

private:
	Box* target;
	std::unique_ptr<Box::ColorTrait> pColorTrait;
};

class ReverseGravity : public Action
{
public:
	void Do( std::vector<std::unique_ptr<Box>>& boxes,b2World& world ) override
	{
		world.SetGravity( -world.GetGravity() );
	}
};

class Push : public Action
{
public:
	Push( Box* target,float factor = 0.5f )
		:
		target( target ),
		factor( factor )
	{
	}
	void Do( std::vector<std::unique_ptr<Box>>& boxes,b2World& world ) override
	{
		target->ApplyLinearImpulse( target->GetVelocity() * factor );
	}

private:
	Box* target;
	float factor;
};

class Consume : public Action
{
public:
	Consume( Box* eater,Box* food )
		:
		eater( eater ),
		food( food )
	{
	}
	void Do( std::vector<std::unique_ptr<Box>>& boxes,b2World& world ) override
	{
		boxes.push_back( eater->GetExpanded( world,food->GetSize() * 0.15f ) );
		food->MarkForDeath();
	}
private:
	Box* eater;
	Box* food;
};

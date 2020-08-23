#pragma once
#include "Box.h"

class RedTrait : public Box::ColorTrait
{
public:
	std::unique_ptr<ColorTrait> Clone() const override
	{
		return std::make_unique<RedTrait>();
	}
	Color GetColor() const override
	{
		return Colors::Red;
	}
};

class GreenTrait : public Box::ColorTrait
{
public:
	std::unique_ptr<ColorTrait> Clone() const override
	{
		return std::make_unique<GreenTrait>();
	}
	Color GetColor() const override
	{
		return Colors::Green;
	}
};

class BlueTrait : public Box::ColorTrait
{
public:
	std::unique_ptr<ColorTrait> Clone() const override
	{
		return std::make_unique<BlueTrait>();
	}
	Color GetColor() const override
	{
		return Colors::Blue;
	}
};

class YellowTrait : public Box::ColorTrait
{
public:
	std::unique_ptr<ColorTrait> Clone() const override
	{
		return std::make_unique<YellowTrait>();
	}
	Color GetColor() const override
	{
		return Colors::Yellow;
	}
};

class WhiteTrait : public Box::ColorTrait
{
public:
	std::unique_ptr<ColorTrait> Clone() const override
	{
		return std::make_unique<WhiteTrait>();
	}
	Color GetColor() const override
	{
		return Colors::White;
	}
};

class CyanTrait : public Box::ColorTrait
{
public:
	std::unique_ptr<ColorTrait> Clone() const override
	{
		return std::make_unique<CyanTrait>();
	}
	Color GetColor() const override
	{
		return Colors::Cyan;
	}
};
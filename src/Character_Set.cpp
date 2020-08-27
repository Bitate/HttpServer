#include "Character_Set.hpp"

URI::Character_Set::~Character_Set()
{
}

URI::Character_Set::Character_Set()
{
}

URI::Character_Set::Character_Set(char c)
{
	characterSet.insert(c);
}

URI::Character_Set::Character_Set(std::initializer_list<const Character_Set> characterSets)
{
	for (auto singleCharacterSet = characterSets.begin(); singleCharacterSet != characterSets.end(); ++singleCharacterSet)
	{
		characterSet.insert(singleCharacterSet->characterSet.begin(), singleCharacterSet->characterSet.end());
	}
}

URI::Character_Set::Character_Set(char first, char last)
{
	// if first char greater than last char
	// swap position
	if (first > last)
	{
		std::swap(first, last);
	}
	
	// insert each char into characterSet
	for (char c = first; c < last + 1; ++c)
	{
		characterSet.insert(c);
	}
}

bool URI::Character_Set::is_contains(char c) const
{
	if (characterSet.find(c) != characterSet.cend())
	{
		return true;
	}
	return false;
}
#pragma once
//Darn it, C++. Remove element from vector.
//https://stackoverflow.com/questions/3385229/c-erase-vector-element-by-value-rather-than-by-position
template<typename T>
void filterOut(std::vector<T> vec, T elem);
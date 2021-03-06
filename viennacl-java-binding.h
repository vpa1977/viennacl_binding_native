#ifndef VIENNACL_JAVA_BINDING_HPP

#define VIENNACL_JAVA_BINDING_HPP
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the VIENNACLJAVABINDING_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// VIENNACLJAVABINDING_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef VIENNACLJAVABINDING_EXPORTS
#define VIENNACLJAVABINDING_API __declspec(dllexport)
#else
#define VIENNACLJAVABINDING_API __declspec(dllimport)
#endif

#define NUM_DATA_TYPE double

#ifdef VIENNACL_WITH_HSA
void free_global(void* free_pointer);
void*  malloc_global(size_t sz);
#endif

#endif

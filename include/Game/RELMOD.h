#ifndef _RELMOD_H_
#define _RELMOD_H_

void RELMOD_RelocModulePointers(int baseaddr, int offset, int *relocs);
void RELMOD_InitModulePointers(int baseaddr, int *relocs);

#endif
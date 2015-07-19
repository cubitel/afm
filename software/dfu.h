
#ifndef DFU_H_
#define DFU_H_

int dfuOpen(void);
int dfuDownload(char *filename);
void dfuClose(void);

#endif /* DFU_H_ */

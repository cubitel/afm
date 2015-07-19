#ifndef IMAGE_H_
#define IMAGE_H_

#include <stddef.h>
#include <stdint.h>
#include <string>

class AFMImage {
private:
	uint16_t width;
	uint16_t height;
	uint8_t *image;
public:
	AFMImage(void);
	~AFMImage(void);
	int SaveAsGSF(std::string filename);
};


#endif /* IMAGE_H_ */

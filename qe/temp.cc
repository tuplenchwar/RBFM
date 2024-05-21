#include <stdio.h>

int main() {
	FILE *file;
	char buffer[5000]; // Adjust this size as needed

	file = fopen("left", "rb");
	if (file == NULL) {
    	printf("Failed to open file\n");
    	return 1;
	}

	while (!feof(file)) {
    	size_t bytesRead = fread(buffer, 1, sizeof(buffer), file);
    	// Process the read data here...
    	// For example, print it to stdout
    	for (size_t i = 0; i < bytesRead; i++) {
        	printf("%c", buffer[i]);
    	}
	}

	fclose(file);

	return 0;
}


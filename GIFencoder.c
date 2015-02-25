#include "GIFencoder.h"
#include "math.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "BitFile.h"

typedef struct pNode* dictHead;

typedef struct pNode {
  int index;
  int length;
  char *code;
  dictHead next;
}pixelNode;

dictHead createHead (void) {
  dictHead aux;
  aux = (dictHead) malloc (sizeof (pixelNode));

  if (aux != NULL) {
    int i = 0;
    aux->index = 0;
    aux->next = NULL;
  }
 return aux;
}

void newElement (dictHead dictActual, int index,char* code, int length) {
 dictHead newNo;
 newNo = (dictHead) malloc (sizeof (pixelNode));

 if (newNo != NULL) {
  newNo->index = index;
  newNo->code = malloc(sizeof(char)*strlen(code));
  newNo->code = code;
  newNo->next = NULL;
  newNo->length = length;
  dictActual->next = newNo;
 }
}

void LZWCompress(FILE * file, char minCodeSize, char *sourceCode, int maxSource){

  dictHead dictBase = createHead();
  dictHead finalNode = dictBase;

  bitStream *finalStream = bitFile(file);

  int clearCode = pow(2,minCodeSize);
  int h = 0;
  char * fill;

  while(h < clearCode){

    fill = malloc(sizeof(int));
    fill[0] = h;
    newElement(finalNode, h, fill, 1);

    finalNode = finalNode->next;
    h++;
  }

  fill = malloc(sizeof(int));
  fill[0] = clearCode;
  newElement(finalNode, clearCode, fill, 1);
  finalNode = finalNode->next;

  fill = malloc(sizeof(int));
  fill[0] = clearCode+1;
  newElement(finalNode, clearCode + 1, fill, 1);
  finalNode = finalNode->next;

  dictHead inicialNode = dictBase->next;

  h = 0;

  int currentPixel = 0;
  int finalCode = 0;
  int currentCodeIndex = finalNode->index + 1;
  int sourceIncr = 1;

  while(currentPixel != maxSource) {

    dictHead actualNode = inicialNode;
    
    int maxLength = 0;
    int first = 1;
    int length = 0;

    while(actualNode->index != currentCodeIndex) {
    
        int j = 0;   
        int valid = 1;
        length = 0;
      
      if(actualNode->length > maxLength){

        while(j != (actualNode->length) && valid == 1) {

          if(actualNode->code[j] !=  sourceCode[currentPixel + j] || currentPixel + j == maxSource ){
                valid = 0;
          	 }
          j++; 
          length++;
        }
      }
 				
      if(valid == 1 && length > maxLength && first == 1) {
      		
        finalCode = actualNode->index; 
        maxLength = length; 
        
        if(currentCodeIndex < 4096 && currentPixel + length < maxSource){

          char *newCode = malloc((actualNode->length + 1)*sizeof(int));
          int k = 0;

          while( k < actualNode->length){

          		newCode[k] = actualNode->code[k];
          		k++;
          }
         
          newCode[k] = sourceCode[currentPixel + length];
          newElement(finalNode, currentCodeIndex, newCode, actualNode->length + 1); 
          finalNode = finalNode->next;
        }

        sourceIncr = length;
        first = 0;
      }

        else if(valid == 1 && length > maxLength) {
      		finalCode = actualNode->index;

          if(currentCodeIndex < 4096){
						char *newCode = malloc((actualNode->length + 1)*sizeof(int));
            int k = 0;
            while( k < actualNode->length){
          		newCode[k] = actualNode->code[k];
          		k++;
            }

            newCode[k] = sourceCode[currentPixel + length];
            finalNode->code = newCode; 
            finalNode->length = actualNode->length+1;
          }

          sourceIncr = length;
        }
      		
       actualNode = actualNode->next;     
    }
    
    // DEBUG
    // printf("PIXEL = %d FINALCODE =  %d, NumBits %d; NOVA ENTRADA %d COM CODIGO ",currentPixel,finalCode, numBits(currentCodeIndex + 1),finalNode->index);
    // int u = 0; 
    // while(u < finalNode->length){
    // 	printf("%d", finalNode->code[u]);
    // 	u++;
    // }
    // printf("\n");    
    // usleep(15000);

    if(currentPixel == 0)
    	 writeBits(finalStream, clearCode, numBits(currentCodeIndex));
 	
    	writeBits(finalStream, finalCode, numBits(currentCodeIndex-1));
  
    if (currentPixel == maxSource)
      writeBits(finalStream, clearCode+1, numBits(currentCodeIndex-1));

    if(currentCodeIndex < 4095)
     currentCodeIndex++;

    currentPixel = currentPixel + sourceIncr;
    
  }

  flush(finalStream);
}



// conversão de um objecto do tipo Image numa imagem indexada
imageStruct* GIFEncoder(unsigned char *data, int width, int height) {
	
	imageStruct* image = (imageStruct*)malloc(sizeof(imageStruct));
	image->width = width;
	image->height = height;

	//converter para imagem indexada
	RGB2Indexed(data, image);

	return image;
}
		
//conversão de lista RGB para indexada: máximo de 256 cores
void RGB2Indexed(unsigned char *data, imageStruct* image) {
	int x, y, colorIndex, colorNum = 0;
	char *copy;

	image->pixels = (char*)calloc(image->width*image->height, sizeof(char));
	image->colors = (char*)calloc(MAX_COLORS * 3, sizeof(char));
	
	
	for (x = 0; x < image->width; x++) {
		for (y = 0; y < image->height; y++) {
			for (colorIndex = 0; colorIndex < colorNum; colorIndex++) {
				if (image->colors[colorIndex * 3] == (char)data[(y * image->width + x)*3] && 
					image->colors[colorIndex * 3 + 1] == (char)data[(y * image->width + x)*3 + 1] &&
					image->colors[colorIndex * 3 + 2] == (char)data[(y * image->width + x)*3 + 2])
					break;
			}

			if (colorIndex >= MAX_COLORS) {
				printf("Demasiadas cores...\n");
				exit(1);
			}

			image->pixels[y * image->width + x] = (char)colorIndex;

			if (colorIndex == colorNum) 
			{
				image->colors[colorIndex * 3]	  = (char)data[(y * image->width + x)*3];
				image->colors[colorIndex * 3 + 1] = (char)data[(y * image->width + x)*3 + 1];
				image->colors[colorIndex * 3 + 2] = (char)data[(y * image->width + x)*3 + 2];
				colorNum++;
			}
		}
	}

	//define o número de cores como potência de 2 (devido aos requistos da Global Color Table)
	image->numColors = nextPower2(colorNum);

	//refine o array de cores com base no número final obtido
	copy = (char*)calloc(image->numColors*3, sizeof(char));
	memset(copy, 0, sizeof(char)*image->numColors*3);
	memcpy(copy, image->colors, sizeof(char)*colorNum*3);
	image->colors = copy;

	image->minCodeSize = numBits(image->numColors - 1);
	if (image->minCodeSize == (char)1)  //imagens binárias --> caso especial (pág. 26 do RFC)
		image->minCodeSize++;
}
	
		
//determinação da próxima potência de 2 de um dado inteiro n
int nextPower2(int n) {
	int ret = 1, nIni = n;
	
	if (n == 0)
		return 0;
	
	while (n != 0) {
		ret *= 2;
		n /= 2;
	}
	
	if (ret % nIni == 0)
		ret = nIni;
	
	return ret;
}
	
	
//número de bits necessário para representar n
char numBits(int n) {
	char nb = 0;
	
	if (n == 0)
		return 0;
	
	while (n != 0) {
		nb++;
		n /= 2;
	}
	
	return nb;
}


//---- Função para escrever imagem no formato GIF, versão 87a
//// COMPLETAR ESTA FUNÇÃO
void GIFEncoderWrite(imageStruct* image, char* outputFile) {
	
	FILE* file = fopen(outputFile, "wb");
	char trailer;

	writeGIFHeader(image, file);
	writeImageBlockHeader(image, file);
	LZWCompress(file, image->minCodeSize ,image->pixels, image->width*image->height);

	fprintf(file, "%c", (char)0x00);
	fprintf(file, "%c", (char)0x3b);
	
	fclose(file);
}
	
	
//--------------------------------------------------
//gravar cabeçalho do GIF (até global color table)
void writeGIFHeader(imageStruct* image, FILE* file) {

	int i;
	char toWrite, GCTF, colorRes, SF, sz, bgci, par;

	//Assinatura e versão (GIF87a)
	char* s = "GIF87a";
	for (i = 0; i < (int)strlen(s); i++)
		fprintf(file, "%c", s[i]);	

	//Ecrã lógico (igual à da dimensão da imagem) --> primeiro o LSB e depois o MSB
	fprintf(file, "%c", (char)( image->width & 0xFF));
	fprintf(file, "%c", (char)((image->width >> 8) & 0xFF));
	fprintf(file, "%c", (char)( image->height & 0xFF));
	fprintf(file, "%c", (char)((image->height >> 8) & 0xFF));
	
	//GCTF, Color Res, SF, size of GCT
	GCTF = 1;
	colorRes = 7;  //número de bits por cor primária (-1)
	SF = 0;
	sz = numBits(image->numColors - 1) - 1; //-1: 0 --> 2^1, 7 --> 2^8
	toWrite = (char) (GCTF << 7 | colorRes << 4 | SF << 3 | sz);
	fprintf(file, "%c", toWrite);

	//Background color index
	bgci = 0;
	fprintf(file, "%c", bgci);

	//Pixel aspect ratio
	par = 0; // 0 --> informação sobre aspect ratio não fornecida --> decoder usa valores por omissão
	fprintf(file, "%c",par);

	//Global color table
	for (i = 0; i < image->numColors * 3; i++)
		fprintf(file, "%c", image->colors[i]);
}

void writeImageBlockHeader(imageStruct* image, FILE* file) {

	fprintf(file, "%c", 0x2c);// Image Separator (0x2c)
	fprintf(file, "%c%c", 0x0000, 0x0000);// Image Left Position
	fprintf(file, "%c%c", 0x0000, 0x0000);// Image Top Position

	fprintf(file, "%c", (char)( image->width & 0xFF));
	fprintf(file, "%c", (char)((image->width >> 8) & 0xFF));
	fprintf(file, "%c", (char)( image->height & 0xFF));
	fprintf(file, "%c", (char)((image->height >> 8) & 0xFF));

	char lctf = 0 << 7;   // Local Color Table present
  char iflag = 0 << 6;  // Image interlaced
  char sflag = 0 << 5;  // Local Color Table sorted

  char s_lct = 0;       //Size of Local Color Table
 
  fprintf(file, "%c", (lctf | iflag | sflag | s_lct));
  fprintf(file, "%c", image->minCodeSize);
}
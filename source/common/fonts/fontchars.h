

// This is a font character that loads a texture and recolors it.
class FFontChar1 : public FTexture
{
public:
   FFontChar1 (FTexture *sourcelump);
   void Create8BitPixels(uint8_t *) override;
   void SetSourceRemap(const uint8_t *sourceremap)  {  SourceRemap = sourceremap;  }
   const uint8_t *ResetSourceRemap() { auto p = SourceRemap; SourceRemap = nullptr; return p; }
   FTexture *GetBase() const { return BaseTexture; }

protected:

   FTexture *BaseTexture;
   const uint8_t *SourceRemap;
};

// This is a font character that reads RLE compressed data.
class FFontChar2 : public FTexture
{
public:
	FFontChar2 (TArray<uint8_t>& sourceData, int sourcepos, int width, int height, int leftofs=0, int topofs=0);

	void Create8BitPixels(uint8_t*) override;
	FBitmap GetBgraBitmap(const PalEntry* remap, int* ptrans) override;
	void SetSourceRemap(const uint8_t *sourceremap);

protected:
	TArray<uint8_t>& sourceData;
	int SourcePos;
	const uint8_t *SourceRemap;
};

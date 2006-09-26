static HANDLE _fonts_list_lock;
static vzHash<vzTTFont*> vzTTFontList;
static vzTTFont* get_font(char* name,long height)
{
	// lock list
	WaitForSingleObject(_fonts_list_lock,INFINITE);

	// build key name is 
	char key[1024];
	sprintf(key,"'%s'-'%d'",name,height);

	// try to find object
	vzTTFont* temp;
	if(temp = vzTTFontList.find(key))
	{
		ReleaseMutex(_fonts_list_lock);
		return temp;
	};
	
	// object not found - create and load
	temp = new vzTTFont(name,height);

	// check if object is ready - success font load
	if(!temp->ready())
	{
#ifdef _DEBUG
		printf("FAILED(not loaded)\n");
#endif

		delete temp;
		ReleaseMutex(_fonts_list_lock);
		return NULL;
	};

	// save object in hash
	vzTTFontList.push(key,temp);

	ReleaseMutex(_fonts_list_lock);

#ifdef _DEBUG
		printf("OK(added)\n");
#endif

	
	return temp;
};

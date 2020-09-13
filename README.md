# pyDev4G2S
The goal of this G2S extension is to quickly prototype new sequential simulation algorithms.

## To install
To install, clone the repository in the `build/extension` folder of G2S.
Then execute `make extension` in  `build` of G2S.

## How to use
To use the code, setup `'-a','pyDev'`, the setup the python code `'-pa',pythonAlgo`.
`pythonAlgo` is a string structured as `'path/to/the/fileWithoutExtension:className'`
If `'className'` is by default the same name as the directory. And the default path is to the `PyDevAlgo` directory in the build folder.

## Structure of a class
```
class nameOfTheClass:
	def __init__(self,TIs,kernel,di,dt,path):
		# all parameters are read-only
		# keep simple operation in this section
		#TIs			==> list of training images if available
		#kernel			==> kernel if available
		#di				==> original destination image
		#dt				==> list of data type
		#path			==> the path, if explicitly set at call time
		pass

	def build(self,n):
		# use this section for more complex initialization
		#n				==> the maximum number neighbors as set in the call
		pass

	def sample(self,coordinates,values,randUniform,indexInThePath,threadId, fullStationary, variableOfInterest, idTI4Sampling):
		# return a sample value as a list of multivariate and index value
		# coordinates			==> list of coordinates
		# values				==> list of values
		# randUniform			==> a random value uniform between 0 and 1
		# indexInThePath		==> index in the path
		# threadId				==> thread id for multithreading, to not use currently !
		# fullStationary		==> boolean about stationarity
		# variableOfInterest	==> the variable to simulate in case of full simulation
		# idTI4Sampling			==> index to choose the training image
		return (aSampleAsAList,AnInfromativeIndex);

	def __del__(self):
		# if need to destroy any element
		pass
```

## Examples

### Exmaple SGS
`[data,t]=g2s('-a','pyDev','-pa','sgs','-di',destination,'-dt',zeros(1,1),'-n',50);
imagesc(data);`

### Exmaple SGS with Constant path
`[data,t]=g2s('-a','pyDev','-pa','sgsCP','-di',destination,'-dt',zeros(1,1),'-n',50,'-sp',path);
imagesc(data);`

### Example SGS with Constant path and TI as model
```
kernel=exp(-0.01*bwdist(padarray(1,[100,100])));
	
[data,t]=g2s('-a','pyDev','-pa','sgsTiCP','-di',destination,'-dt',zeros(1,1),'-n',50,'-sp',path,'-ti',kernel);
imagesc(data);
```

### Example SGS with Constant path and multiple TI as model and non-stationarity
```
kernels={};
for alpha=2.^(-12:0.5:-5)
	kernels{numel(kernels)+1}=exp(-alpha*bwdist(padarray(1,[100,100])));
end
	
indexImage=floor((0:size(destination,1)-1)/size(destination,1)*numel(kernels)).*ones(size(destination,2),1);
	
[data,t]=g2s('-a','pyDev','-pa','sgsTiCP','-di',destination,'-dt',zeros(1,1),'-n',50,'-sp',path,'-ti',kernels{:},'-ii',indexImage);
imagesc(data);
```

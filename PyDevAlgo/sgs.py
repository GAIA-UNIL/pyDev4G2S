import numpy
from scipy.stats import norm

class sgs:
	def __init__(self,TIs,kernel,di,dt,path):
		pass

	def build(self,n):
		self.covFunction=lambda x:numpy.exp(-0.01*x);
		pass

	def sample(self,coordinates,values,randUniform,indexInThePath,threadId, fullStationary, variableOfInterest, idTI4Sampling):
		noise=norm.ppf(randUniform);
		values=numpy.array(values)
		if(values.size<5):
			return (noise, 0);
		coordinates=numpy.array(coordinates);
		
		# remove nans
		mask=numpy.logical_not(numpy.isnan(values));
		inputVect=coordinates[mask[:,0],:];
		values=values[mask];

		MD=inputVect[:,None,:]-inputVect[None,:,:];
		C=self.covFunction(numpy.linalg.norm(MD,axis=-1));
		C=numpy.block([[C,numpy.ones((C.shape[0],1))],[numpy.ones((1,C.shape[0])),numpy.zeros((1,1))]]);
		C0=numpy.block([[self.covFunction(numpy.linalg.norm(inputVect[:,:],axis=-1,keepdims=True))],[numpy.ones((1,1))]]);
		coef= numpy.linalg.solve(C,C0);
		dev=numpy.sqrt(numpy.abs(1-numpy.sum(C0*coef)));
		return ([(coef[:-1,0]*values).sum()+noise*dev],1);
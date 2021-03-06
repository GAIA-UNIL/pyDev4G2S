import numpy
from scipy.stats import norm
from hashlib import blake2b
import os
class sgsCP:
	def __init__(self,TIs,kernel,di,dt,path):
		if(path is not None):
			self.coefFileName='./data/coef_'+blake2b(path.data).hexdigest()[:25];
		pass

	def __del__(self):
		self.file.close();

	def build(self,n):
		self.covFunction=lambda x:numpy.exp(-0.01*x);
		self.coefFileName=self.coefFileName+"_"+str(n[0]);
		if os.path.isfile(self.coefFileName):
			self.file=open(self.coefFileName, "rb"); 
			self.needComputation=False;
		else:
			self.file=open(self.coefFileName, "wb");
			self.needComputation=True;
		pass

	def sample(self,coordinates,values,randUniform,indexInThePath,threadId, fullStationary, variableOfInterest, idTI4Sampling):
		noise=norm.ppf(randUniform);
		values=numpy.array(values)
		if(values.size<2):
			return (noise, 0);
		if self.needComputation:
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
			self.file.write(dev)
			self.file.write(coef[:-1,0].data)
			return ([(coef[:-1,0]*values).sum()+noise*dev],1);
		else:
			values[0]=noise;
			coefs=numpy.frombuffer(self.file.read(values.shape[0]*8), dtype=numpy.double);
			return (numpy.sum(values.flatten()*coefs),2)



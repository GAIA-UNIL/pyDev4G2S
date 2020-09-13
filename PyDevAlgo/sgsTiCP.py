import numpy
from scipy.stats import norm
from hashlib import blake2b
import os

class sgsTiCP:
	def __init__(self,TIs,kernel,di,dt,path):
		self.needComputation=True;
		self.file=None;
		self.cov=TIs;
		if(path is not None):
			self.coefFileName='./data/coef_'+blake2b(path.data).hexdigest()[:25];
		pass

	def __del__(self):
		if(self.file is not None):
			self.file.close();

	def build(self,n):
		self.center=numpy.array(self.cov[0].shape)//2;
		self.innerFunction=lambda x,imId:0 if((numpy.abs(x)-self.center).max()>0) else self.cov[imId][tuple(x+self.center)];
		self.covFunction=lambda y,imId:numpy.apply_along_axis( self.innerFunction, -1, y, imId);
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
			C=self.covFunction(MD,numpy.maximum(idTI4Sampling,0));
			C=numpy.block([[C,numpy.ones((C.shape[0],1))],[numpy.ones((1,C.shape[0])),numpy.zeros((1,1))]]);
			C0=self.covFunction(inputVect[:,:],numpy.maximum(idTI4Sampling,0));
			C0=numpy.block([[C0[:,None]],[numpy.ones((1,1))]]);
			coef= numpy.linalg.solve(C,C0);
			dev=numpy.sqrt(numpy.abs(1-numpy.sum(C0*coef)));
			if(self.file is not None):
				self.file.write(dev)
				self.file.write(coef[:-1,0].data)
			return ([(coef[:-1,0]*values).sum()+noise*dev],1);
		else:
			values[0]=noise;
			coefs=numpy.frombuffer(self.file.read(values.shape[0]*8), dtype=numpy.double);
			return (numpy.sum(values.flatten()*coefs),2)



# Intel One API Code Maven Hackathonn
<img src="usage.gif">

This is my submission for the Intel OneAPI Code Maven Hackahton conducted by Techgig. The problem statement was to use the OneAPI library to implement a Stock Prediction algorithm that uses Monte Carlo Sampling. 

I implemented the paper [Modelling the probability of future stock returns- TOBIAS BRODD, ADRIAN DJERF](https://www.diva-portal.org/smash/get/diva2:1214365/FULLTEXT01.pdf).

I also implemented the bayesian inference method described in this talk [here](https://www.youtube.com/watch?v=R2DpQRt17oc). I tried creating a bayesian inference engine with [Hamiltonian Monte Carlo](https://bjlkeng.io/posts/hamiltonian-monte-carlo/) but it didn't work(check this [commit](https://bjlkeng.io/posts/hamiltonian-monte-carlo/) to try it for yourself). I also implement [Metropolis Hasting](https://github.com/professorcode1/IntelHack/commit/f8ae14ac6d451212227b13aaed593518cdcbf05f) for the same but that didn't work either.

To build locally on windows
<ul>
<li>Install OneAPI DPC and DPL libraries</li>
<li>Install CPR and boost-date-time (using vcpkg is most convinient)</li>
</ul>

To run it just download it from [here](https://drive.google.com/drive/folders/1DppgDQA_yMxfs1gvl6O6Pi4eSjGHY4kV?usp=sharing), generate a API from [Rapid API](pidapi.com/alphavantage/api/alpha-vantage). Put it in a file an use it on the first screen.
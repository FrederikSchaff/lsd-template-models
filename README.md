# lsd-template-models
Template models that make use of the lsd-modules.

## Population

A simple implementation of a population model in LSD (GIS-BETA). 

**Modules used:** pop, validate, PajekFromCpp

## Pajek_Schelling

Using the GIS implementation of the Schelling model and Pajek to create a visualisation as network that changes in time. The created dynamic network in .paj format can be used with PajekToSVGAnim by  Darko Brvar, see http://mrvar.fdv.uni-lj.si/pajek/ (scroll down to supporting programmes)

**modules used:** PajekFromCpp

### Usage

- Make sure you use LSD with a GIS version.
  (currently branch GIS-BETA: https://github.com/marcov64/Lsd/tree/GIS-beta)
- Clone the repo into your "Work" directory in LSD.
- Initialise the submodules.



```
Copyright:  Frederik Schaff, 2018
Version: 0.1
Licence: MIT
```


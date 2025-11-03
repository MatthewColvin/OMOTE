# Actions 

Actions are a way to command Omote to do some sort of action.

## JSON Structure

1) IR Action
```
{
    "type": "IRAction",
    "name": "RokuHome",
    "data": {
        "protocol": "NEC",
        "data": "0x5743C03F"
    }
}
```
   1) **type** - Types in ActionTypes.hpp
   2) **name** - User defined name
   3) **data** - A RapidJson::Value object passed to the IAction subclass(after Validation)

If you want to know what goes in data object of a specific Action type go to the File Associated with the ActionType via the ActionFactory. 
For example IRAction.cpp has a Schema String that describes the value that goes with the data key. 


## Adding new action

1) Extend the IAction class into something that allows Omote to perform some specific behavior. 

3) Create a new ActionType in ActionTypes.hpp

2) You can then register this with the ActionFactory at the top of the Actions.cpp (see example in IRAction.cpp) 
   1) This factory registration attempts to simplify validation of the data in your schema. 
   2) You will define a RapidJson Schema that represents the data JSON object needed to construct your action.
   3) Then you will need to define how to construct your device given validated JSON. 
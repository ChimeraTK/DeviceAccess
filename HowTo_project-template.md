This file descibes the intended way to use the template project.

## Concept
The project template is merged via git. The idea is to have the whole
project-template repository merged into the project which uses it, incl.
all the history. Like this updates and improvements in the template
scripts can easily be propagated to all client projects by simply
merging the head of project template.

## Instructions

### Initial import and getting of the project template
In your local git repository, issue:
  ```
    $ git fetch https://github.com/ChimeraTK/project-template.git
    $ git merge FETCH_HEAD --allow-unrelated-histories
```
To pull in an update of the project template, you can leave out the `--allow-unrelated-histories` flag.

Note: you can access the files provided in the cmake directory from your 
CMakeLists.txt after appending this location to your `CMAKE_MODULE_PATH`
variable

### Updating/Improving the project template

  - Never write to the project template from one of the client projects!  If
    you have improvements or additions to the project template, check out
    the project-template repository separately, make the modifications and
    commit/push them.
  
  - Be careful not to break functionality for other project which are using
    the template!
  
  - After the project template is updated you can merge the changes into
    your project as described in the section above. (Getting updates of project template)
  
  - Note: For testing you might want to add your local repository of
    project-template as a remote to your project, so you don't have to push
    untested changes. Be careful that everything is pushed to github when
    you are done in order not to lose consistency of the repositories.


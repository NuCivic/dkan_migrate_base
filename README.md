DKAN Migrate Base
=================
This provides base classes for common DKAN migrations (ie imports or harvests).

The base classes will import Datasets, resources, tags, groups, and users from a CKAN site.

To use, create your own migration and create a class that inherits MigrateCkanDatasetBase (code examples coming soon) or change the endpoint ``$this->endpoint = 'http://demo.ckan.org/api/3/action/';`` to your favorite CKAN or DKAN site.

### Migrate Module
This uses the Migrate module which is well documented: https://www.drupal.org/node/415260

Once setup, migrations can be run through the user interface:

![screen shot 2014-08-19 at 9 49 02 am](https://cloud.githubusercontent.com/assets/512243/3968050/13c20b04-27b3-11e4-9365-3567a9adcc2d.png)

through the command line, or run periodically.


TO-DO:
* finish base node fields
* add tags, groups
* document adding endpoints and fields
